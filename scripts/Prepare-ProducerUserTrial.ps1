[CmdletBinding()]
param([ValidateSet('Prepare','Install','Restore')][string]$Mode='Prepare',[string]$TrialDirectory,[string]$AsciiAliasRoot)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$executionIdentity=[Security.Principal.WindowsIdentity]::GetCurrent()
$trialRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))
$base32=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::CurrentUser,[Microsoft.Win32.RegistryView]::Registry32)
$base64=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::CurrentUser,[Microsoft.Win32.RegistryView]::Registry64)
function Assert-TrialPath([string]$Directory){
    $resolved=[IO.Path]::GetFullPath($Directory)
    if(-not $resolved.StartsWith($trialRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Trial directory must be beneath work/integration/user-trial'}
    return $resolved
}
function Decode-Value($Entry){
    $bytes=[Convert]::FromHexString($Entry.hex)
    switch([int]$Entry.type){
        1 {return [Text.Encoding]::Unicode.GetString($bytes).TrimEnd([char]0)}
        2 {return [Text.Encoding]::Unicode.GetString($bytes).TrimEnd([char]0)}
        3 {return ,$bytes}
        4 {if($bytes.Length -ne 4){throw 'Invalid DWORD'};return [BitConverter]::ToInt32($bytes,0)}
        7 {return ,([Text.Encoding]::Unicode.GetString($bytes).TrimEnd([char]0).Split([char]0))}
        11 {if($bytes.Length -ne 8){throw 'Invalid QWORD'};return [BitConverter]::ToInt64($bytes,0)}
        default {throw "Unsupported registry type: $($Entry.type)"}
    }
}
function Read-OwnedKeys($Roots){
    return @(foreach($entry in $Roots){
        $base=if($entry.view -eq 32){$base32}else{$base64}
        $key=$base.OpenSubKey($entry.path)
        [ordered]@{view=$entry.view;path=$entry.path;exists=$null -ne $key}
        if($key){$key.Dispose()}
    })
}
try{
    if($Mode -eq 'Prepare'){
        if($TrialDirectory){throw 'Prepare creates its own new trial directory'}
        $run=Join-Path $trialRoot ([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
        New-Item -ItemType Directory -Path $run | Out-Null
        $app=Join-Path $run 'app'
        Copy-Item -LiteralPath (Join-Path $repo 'work/producer/app') -Destination $app -Recurse
        $files=@(foreach($file in Get-ChildItem -LiteralPath $app -File){
            $source=Join-Path $repo ('work/producer/app/'+$file.Name)
            $hash=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()
            if((Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant() -ne $hash){throw 'Original copy mismatch'}
            [ordered]@{name=$file.Name;bytes=$file.Length;sha256=$hash}
        })
        $launchApp=$app
        if($AsciiAliasRoot){
            $aliasRoot=[IO.Path]::GetFullPath($AsciiAliasRoot)
            if($aliasRoot -match '[^\x00-\x7F]' -or -not (Test-Path -LiteralPath $aliasRoot -PathType Container)){throw 'An existing writable ASCII alias root is required'}
            $launchApp=Join-Path $aliasRoot ('producer-reference-'+[IO.Path]::GetFileName($run))
            if(Test-Path -LiteralPath $launchApp){throw 'ASCII alias must be new'}
            if((Assert-TrialPath $app) -ine $app){throw 'Junction target outside trial'}
            New-Item -ItemType Junction -Path $launchApp -Target $app | Out-Null
            foreach($file in $files){if((Get-FileHash -LiteralPath (Join-Path $launchApp $file.name)).Hash.ToLowerInvariant() -ne $file.sha256){throw 'ASCII alias identity mismatch'}}
        }
        $registration=Get-Content -LiteralPath (Join-Path $repo 'docs/analysis/registration-capture.json') -Raw|ConvertFrom-Json
        if($registration.components.Count -ne 10 -or ($registration.components|Where-Object {-not $_.complete})){throw 'All ten observed Components registrations required'}
        $entries=@();$skipped=@();$roots=@{}
        foreach($capture in $registration.captures|Where-Object completed){
            $module=Join-Path $app $capture.module
            if((Get-FileHash -LiteralPath $module).Hash.ToLowerInvariant() -ne $capture.sha256){throw 'Captured module identity changed'}
            foreach($value in $capture.exportedValues){
                $destinations=@()
                if($value.path.StartsWith('\HKCR\')){
                    $relative=$value.path.Substring(6)
                    if($relative -eq 'DirectShow' -or $relative.StartsWith('DirectShow\')){
                        $skipped+=@([ordered]@{module=$capture.module;path=$value.path;reason='Existing DirectShow namespace; optional DMO category registration excluded from startup trial'})
                        continue
                    }
                    $rootPart=if($relative.StartsWith('CLSID\')){($relative.Split('\')|Select-Object -First 2)-join '\'}else{$relative.Split('\')[0]}
                    if(-not $rootPart -or $rootPart -eq 'CLSID'){throw 'Invalid private COM root'}
                    $rootPath='Software\Classes\'+$rootPart
                    $roots['32|'+$rootPath]=[ordered]@{view=32;path=$rootPath}
                    $destinations=@([ordered]@{view=32;path='Software\Classes\'+$relative})
                }elseif($value.path.StartsWith('\HKLM\Software\Microsoft\DMUSProducer\')){
                    $relative=$value.path.Substring('\HKLM\Software\Microsoft\DMUSProducer'.Length)
                    foreach($virtualParent in @('Software\Classes\VirtualStore\MACHINE\SOFTWARE\Microsoft\DMUSProducer','Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\Microsoft\DMUSProducer')){
                        $roots['64|'+$virtualParent]=[ordered]@{view=64;path=$virtualParent}
                        $destinations+=@([ordered]@{view=64;path=$virtualParent+$relative})
                    }
                }else{
                    $skipped+=@([ordered]@{module=$capture.module;path=$value.path;reason='Outside captured COM and Producer namespace'})
                    continue
                }
                $hex=$value.hex
                if($value.path -match '^\\HKCR\\CLSID\\\{[^}]+\}\\InProcServer32$' -and $value.name -eq ''){
                    if($value.type -ne 1){throw 'InProcServer32 must be REG_SZ'}
                    # All captured classes are in-process classes from this original module.
                    if(Test-Path -LiteralPath ([string]$value.decoded) -PathType Leaf){
                        $serverHash=(Get-FileHash -LiteralPath ([string]$value.decoded)).Hash.ToLowerInvariant()
                        if($serverHash -ne $capture.sha256){throw 'Captured server module identity mismatch'}
                    }
                    # Some original ANSI registrations contain a garbled directory.
                    # The capture's verified source module, not that broken pathname,
                    # establishes ownership. Register the trial copy by its ASCII alias.
                    $registeredModule=Join-Path $launchApp $capture.module
                    $hex=[Convert]::ToHexString([Text.Encoding]::Unicode.GetBytes($registeredModule+[char]0)).ToLowerInvariant()
                }
                foreach($destination in $destinations){
                    $entry=[ordered]@{view=$destination.view;path=$destination.path;name=$value.name;type=$value.type;hex=$hex;sourceModule=$capture.module;sourcePath=$value.path}
                    $null=Decode-Value $entry
                    $entries+=@($entry)
                }
            }
        }
        $owned=@($roots.Values|Sort-Object view,path)
        $before=@(Read-OwnedKeys $owned)
        if($before|Where-Object exists){throw 'One or more target roots already exist; no registration installed'}
        $mergedClasses=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::ClassesRoot,[Microsoft.Win32.RegistryView]::Registry32)
        try{foreach($entry in $owned|Where-Object view -eq 32){
            $relative=$entry.path.Substring('Software\Classes\'.Length)
            $key=$mergedClasses.OpenSubKey($relative)
            if($key){$key.Dispose();throw "Existing merged COM root: $relative"}
        }}finally{$mergedClasses.Dispose()}
        $plan=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');executionUser=$executionIdentity.Name;executionSid=$executionIdentity.User.Value;trial=$run;app=$app;launchApp=$launchApp;files=$files;ownedRoots=$owned;before=$before;entries=$entries;skipped=$skipped;registrationSourceSha256=(Get-FileHash -LiteralPath (Join-Path $repo 'docs/analysis/registration-capture.json')).Hash.ToLowerInvariant();expectedComponents=10;installed=$false}
        $plan|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $run 'plan.json') -Encoding utf8
        Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'prepare-script.ps1')
        Write-Output "Prepared: $run"
        Write-Output "Absent roots: $($owned.Count); values: $($entries.Count); copied files: $($files.Count)"
        return
    }
    if(-not $TrialDirectory){throw 'Install and Restore require the prepared trial directory'}
    $run=Assert-TrialPath $TrialDirectory
    $planPath=Join-Path $run 'plan.json'
    $plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
    if($plan.executionSid -ne $executionIdentity.User.Value){throw 'Run Install/Restore under the Windows account that prepared the trial'}
    if([IO.Path]::GetFullPath($plan.trial) -ine $run -or (Assert-TrialPath $plan.app) -ine (Join-Path $run 'app')){throw 'Trial identity mismatch'}
    foreach($root in $plan.ownedRoots){
        $validCom=$root.view -eq 32 -and ($root.path -match '^Software\\Classes\\CLSID\\\{[0-9A-Fa-f-]{36}\}$' -or $root.path -match '^Software\\Classes\\[A-Za-z0-9]+\.[A-Za-z0-9.]+$')
        $validStore=$root.view -eq 64 -and $root.path -in @('Software\Classes\VirtualStore\MACHINE\SOFTWARE\Microsoft\DMUSProducer','Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\Microsoft\DMUSProducer')
        if(-not ($validCom -or $validStore)){throw 'Unexpected managed registry root'}
    }
    if($Mode -eq 'Restore'){
        if(-not (Test-Path -LiteralPath (Join-Path $run 'install-state.json'))){throw 'No installation state; no keys removed'}
        $installState=Get-Content -LiteralPath (Join-Path $run 'install-state.json') -Raw|ConvertFrom-Json
        if((Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant() -ne $installState.planSha256){throw 'Trial plan changed after installation; no keys removed'}
        $active=@(Get-Process -Name DMUSProd -ErrorAction SilentlyContinue|Where-Object {$_.Path -ieq (Join-Path $plan.app 'DMUSProd.exe') -or $_.Path -ieq (Join-Path $plan.launchApp 'DMUSProd.exe')})
        if($active){throw 'Trial Producer still running; close it before restoring'}
        foreach($root in $plan.ownedRoots){$base=if($root.view -eq 32){$base32}else{$base64};$base.DeleteSubKeyTree($root.path,$false)}
        $after=@(Read-OwnedKeys $plan.ownedRoots)
        $record=[ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');allOwnedRootsAbsent=-not [bool]($after|Where-Object exists);roots=$after}
        $record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'restore.json') -Encoding utf8
        if(-not $record.allOwnedRootsAbsent){throw 'Registry restoration incomplete'}
        Write-Output "Restored: $run"
        return
    }
    if(Test-Path -LiteralPath (Join-Path $run 'install-state.json')){throw 'This trial already has installation state; do not install twice'}
    $current=@(Read-OwnedKeys $plan.ownedRoots)
    if($current|Where-Object exists){throw 'Target state changed since preparation; no values installed'}
    foreach($file in $plan.files){if((Get-FileHash -LiteralPath (Join-Path $plan.app $file.name)).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Trial app changed since preparation'}}
    # Persist ownership before the first mutation so a partial failure is recoverable.
    $planHash=(Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant()
    [ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;completed=$false;planSha256=$planHash}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'install-state.json') -Encoding utf8
    foreach($entry in $plan.entries){
        if(-not ($plan.ownedRoots|Where-Object {$_.view -eq $entry.view -and ($entry.path -eq $_.path -or $entry.path.StartsWith($_.path+'\'))})){throw 'Value outside managed root'}
        $base=if($entry.view -eq 32){$base32}else{$base64}
        $key=$base.CreateSubKey($entry.path,$true)
        try{$key.SetValue($entry.name,(Decode-Value $entry),[Microsoft.Win32.RegistryValueKind]$entry.type)}finally{$key.Dispose()}
    }
    $verified=0
    foreach($entry in $plan.entries){
        $base=if($entry.view -eq 32){$base32}else{$base64}
        $key=$base.OpenSubKey($entry.path)
        try{
            if(-not $key -or [int]$key.GetValueKind($entry.name) -ne [int]$entry.type){throw 'Installed value type mismatch'}
            $actual=$key.GetValue($entry.name,$null,[Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
            if(($actual|ConvertTo-Json -Compress) -ne ((Decode-Value $entry)|ConvertTo-Json -Compress)){throw 'Installed value content mismatch'}
            ++$verified
        }finally{if($key){$key.Dispose()}}
    }
    [ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;completed=$true;verifiedValues=$verified;ownedRoots=$plan.ownedRoots.Count;planSha256=$planHash}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'install-state.json') -Encoding utf8
    Write-Output "Installed user trial: $run; verified values: $verified"
}finally{$base32.Dispose();$base64.Dispose()}

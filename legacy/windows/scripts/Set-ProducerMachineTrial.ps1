[CmdletBinding()]
param([ValidateSet('Prepare','Install','Restore')][string]$Mode='Prepare',[Parameter(Mandatory)][string]$TrialDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))
$run=[IO.Path]::GetFullPath($TrialDirectory)
if(-not $run.StartsWith($root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Use a prepared user-trial directory'}
$keyPath='Software\Microsoft\DMUSProducer'
$planPath=Join-Path $run 'machine-plan.json'
$statePath=Join-Path $run 'machine-install-state.json'
$base=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryView]::Registry32)
function Decode-MachineValue($Entry){
    $raw=[Convert]::FromHexString($Entry.hex)
    switch([int]$Entry.type){
        1 {return [Text.Encoding]::Unicode.GetString($raw).TrimEnd([char]0)}
        4 {if($raw.Length -ne 4){throw 'Invalid DWORD'};return [BitConverter]::ToInt32($raw,0)}
        default {throw 'Unexpected machine value type'}
    }
}
try{
    if($Mode -eq 'Prepare'){
        if(Test-Path -LiteralPath $planPath){throw 'Machine plan already exists'}
        $userPlan=Get-Content -LiteralPath (Join-Path $run 'plan.json') -Raw|ConvertFrom-Json
        $existing=$base.OpenSubKey($keyPath)
        if($existing){$existing.Dispose();throw 'Machine Producer namespace already exists; no plan prepared'}
        $entries=@(foreach($entry in $userPlan.entries|Where-Object { $_.view -eq 64 -and $_.path.StartsWith('Software\Classes\VirtualStore\MACHINE\SOFTWARE\WOW6432Node\Microsoft\DMUSProducer\') }){
            if(-not $entry.sourcePath.StartsWith('\HKLM\'+$keyPath+'\')){throw 'Unexpected capture source'}
            $item=[ordered]@{path=$entry.sourcePath.Substring('\HKLM\'.Length);name=$entry.name;type=$entry.type;hex=$entry.hex;sourceModule=$entry.sourceModule}
            $null=Decode-MachineValue $item
            $item
        })
        if(-not $entries.Count -or @($entries|Where-Object {$_.path -like '*\Components\*' -and $_.name -eq 'Skip'}).Count -ne 10){throw 'Ten complete Component registrations required'}
        $plan=[ordered]@{schema=1;preparedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;hive='HKEY_LOCAL_MACHINE';view=32;ownedRoot=$keyPath;beforeExists=$false;entries=$entries;userPlanSha256=(Get-FileHash -LiteralPath (Join-Path $run 'plan.json')).Hash.ToLowerInvariant();requiresAdministrator=$true;changesCOMClasses=$false;changesSecuritySettings=$false;installsFonts=$false}
        $plan|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $planPath -Encoding utf8
        Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'machine-script.ps1')
        Write-Output "Prepared machine plan: $planPath; values: $($entries.Count)"
        return
    }
    $identity=[Security.Principal.WindowsIdentity]::GetCurrent()
    if(-not [Security.Principal.WindowsPrincipal]::new($identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)){throw 'Administrator approval required; no machine registration changed'}
    $plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
    if($plan.trial -ine $run -or $plan.ownedRoot -cne $keyPath -or $plan.view -ne 32 -or $plan.beforeExists -ne $false){throw 'Invalid machine plan identity'}
    foreach($entry in $plan.entries){if(-not $entry.path.StartsWith($keyPath+'\',[StringComparison]::OrdinalIgnoreCase)){throw 'Entry outside owned machine root'};$null=Decode-MachineValue $entry}
    if($Mode -eq 'Restore'){
        if(-not (Test-Path -LiteralPath $statePath)){throw 'No install ownership record; no keys removed'}
        $state=Get-Content -LiteralPath $statePath -Raw|ConvertFrom-Json
        if((Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant() -ne $state.planSha256){throw 'Machine plan changed since installation'}
        $base.DeleteSubKeyTree($keyPath,$false)
        $remaining=$base.OpenSubKey($keyPath)
        if($remaining){$remaining.Dispose();throw 'Machine registry restoration incomplete'}
        [ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');ownedRoot=$keyPath;view=32;restoredAbsent=$true}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'machine-restore.json') -Encoding utf8
        Write-Output 'Machine trial namespace restored to absent'
        return
    }
    if(Test-Path -LiteralPath $statePath){throw 'Already has installation state; do not install twice'}
    if((Get-FileHash -LiteralPath (Join-Path $run 'plan.json')).Hash.ToLowerInvariant() -ne $plan.userPlanSha256){throw 'Prepared user plan changed'}
    $existing=$base.OpenSubKey($keyPath)
    if($existing){$existing.Dispose();throw 'Machine namespace no longer absent; no registration changed'}
    $hash=(Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant()
    [ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');completed=$false;planSha256=$hash;ownedRoot=$keyPath;view=32;executionUser=$identity.Name}|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
    foreach($entry in $plan.entries){$key=$base.CreateSubKey($entry.path,$true);try{$key.SetValue($entry.name,(Decode-MachineValue $entry),[Microsoft.Win32.RegistryValueKind]$entry.type)}finally{$key.Dispose()}}
    $verified=0
    foreach($entry in $plan.entries){$key=$base.OpenSubKey($entry.path);try{
        if(-not $key -or [int]$key.GetValueKind($entry.name) -ne [int]$entry.type -or $key.GetValue($entry.name) -cne (Decode-MachineValue $entry)){throw 'Machine registration verification failed'}
        ++$verified
    }finally{if($key){$key.Dispose()}}}
    [ordered]@{finishedUtc=[DateTime]::UtcNow.ToString('o');completed=$true;planSha256=$hash;ownedRoot=$keyPath;view=32;verifiedValues=$verified;executionUser=$identity.Name}|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
    Write-Output "Installed machine trial values: $verified"
}catch{
    [ordered]@{time=[DateTime]::UtcNow.ToString('o');mode=$Mode;error=$_.Exception.Message}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run ('machine-'+$Mode.ToLowerInvariant()+'-error.json')) -Encoding utf8
    throw
}finally{$base.Dispose()}

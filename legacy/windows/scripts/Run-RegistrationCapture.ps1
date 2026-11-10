[CmdletBinding()]
param([string]$Module='SegmentDesigner.ocx',[string]$BuildDirectory='work/build/registration')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if($Module -notmatch '^[A-Za-z0-9]+\.(dll|ocx)$'){throw 'Use one module filename from the original app directory'}
$source=Join-Path $repo ('work/producer/app/'+$Module)
$expected=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()
$pe=Get-Content -LiteralPath (Join-Path $repo ('work/analysis/pe/app__'+$Module+'/pe.json')) -Raw | ConvertFrom-Json
if($expected -ne $pe.sha256 -or -not ($pe.exports|Where-Object name -eq 'DllRegisterServer')){throw 'Known original registration export required'}
$run=Join-Path $repo ('work/integration/registration/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run | Out-Null
$build=if([IO.Path]::IsPathRooted($BuildDirectory)){$BuildDirectory}else{Join-Path $repo $BuildDirectory}
$exe=Join-Path $build 'Release/producer_registration_probe.exe'
$sources=@('tests/native/registration/CMakeLists.txt','tests/native/producer_registration_probe.cpp','scripts/Run-RegistrationCapture.ps1')
function Read-GlobalRegistrationPresence {
    $result=@(foreach($view in @([Microsoft.Win32.RegistryView]::Registry32,[Microsoft.Win32.RegistryView]::Registry64)){
        foreach($hive in @([Microsoft.Win32.RegistryHive]::CurrentUser,[Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryHive]::ClassesRoot)){
            $base=[Microsoft.Win32.RegistryKey]::OpenBaseKey($hive,$view)
            $paths=if($hive -eq [Microsoft.Win32.RegistryHive]::ClassesRoot){@('CLSID\{DFCE860B-A6FA-11D1-8881-00C04FBF8D15}','CLSID\{853BAF7B-D3C8-11D1-88BC-00C04FBF8D15}')}else{@('Software\Microsoft\DMUSProducer')}
            foreach($keyPath in $paths){$key=$base.OpenSubKey($keyPath)
                [ordered]@{hive=$hive.ToString();view=$view.ToString();path=$keyPath;exists=$null -ne $key}
                if($key){$key.Dispose()}}
            $base.Dispose()
        }
    })
    return $result
}
$globalBefore=@(Read-GlobalRegistrationPresence)
ConvertTo-Json -InputObject $globalBefore -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'global-before.json') -Encoding utf8
$identities=@(foreach($item in $sources){
    $snapshot=Join-Path $run ('sources/'+$item)
    New-Item -ItemType Directory -Path (Split-Path $snapshot -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $repo $item) -Destination $snapshot
    [ordered]@{path=$item;sha256=(Get-FileHash -LiteralPath $snapshot).Hash.ToLowerInvariant()}
})
$failure=$null;$process=$null;$exitCode=$null;$timeout=$false
try{
    $arguments=@(('"{0}"' -f $source),$expected,('"{0}"' -f (Join-Path $run 'capture.hiv')))
    $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'capture.jsonl') -RedirectStandardError (Join-Path $run 'stderr.txt')
    $timeout=-not $process.WaitForExit(15000)
    if($timeout){Stop-Process -Id $process.Id;$process.WaitForExit()}
    $process.Refresh();$exitCode=$process.ExitCode
}catch{$failure=$_.Exception.Message}
$after=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()
$globalAfter=@(Read-GlobalRegistrationPresence)
ConvertTo-Json -InputObject $globalAfter -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'global-after.json') -Encoding utf8
$globalSame=($globalBefore|ConvertTo-Json -Depth 5 -Compress) -eq ($globalAfter|ConvertTo-Json -Depth 5 -Compress)
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$record=[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');executionUser=$identity.Name;executionSid=$identity.User.Value;module=$source;sha256=$expected;moduleUnchanged=$after -eq $expected;globalPresenceUnchanged=$globalSame;probeSha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();exitCode=$exitCode;timedOut=$timeout;launchError=$failure;sources=$identities;registryMode='RegLoadAppKey(REG_PROCESS_APPKEY), HKCR/HKLM/HKCU remapped within the capture process';hive=Join-Path $run 'capture.hiv'}
$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding utf8
Write-Output "Evidence: $run"
Write-Output "Exit: $exitCode; timeout: $timeout; launch error: $failure"
if($failure -or $timeout -or $exitCode -ne 0 -or $after -ne $expected -or -not $globalSame){throw 'Registration capture incomplete; evidence retained'}

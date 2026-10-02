[CmdletBinding()]
param([switch]$Visible)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$expected='fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011'
$source=Join-Path $repo 'work/producer/app'
if((Get-FileHash -LiteralPath (Join-Path $source 'DMUSProd.exe')).Hash.ToLowerInvariant() -ne $expected){throw 'Original executable hash mismatch'}
$run=Join-Path $repo ('work/integration/startup/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run -Force | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $run 'app') -Recurse
$identities=foreach($file in Get-ChildItem -LiteralPath $source -File){
    $copied=Join-Path $run ('app/'+$file.Name)
    $hash=(Get-FileHash -LiteralPath $file.FullName).Hash.ToLowerInvariant()
    if((Get-FileHash -LiteralPath $copied).Hash.ToLowerInvariant() -ne $hash){throw "Copy identity mismatch: $($file.Name)"}
    [ordered]@{name=$file.Name;bytes=$file.Length;sha256=$hash}
}
$identities|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'files-before.json') -Encoding utf8
$keys=foreach($view in @([Microsoft.Win32.RegistryView]::Registry32,[Microsoft.Win32.RegistryView]::Registry64)){
    foreach($hive in @([Microsoft.Win32.RegistryHive]::CurrentUser,[Microsoft.Win32.RegistryHive]::LocalMachine)){
        $base=[Microsoft.Win32.RegistryKey]::OpenBaseKey($hive,$view)
        $key=$base.OpenSubKey('Software\Microsoft\DMUSProducer')
        [ordered]@{hive=$hive.ToString();view=$view.ToString();path='Software\Microsoft\DMUSProducer';exists=$null -ne $key}
        if($key){$key.Dispose()};$base.Dispose()
    }
}
$keys|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'registry-before.json') -Encoding utf8
$executable=Join-Path $run 'app/DMUSProd.exe'
$failure=$null;$process=$null;$modules=@()
try{
    $style=if($Visible){'Normal'}else{'Hidden'}
    $process=Start-Process -FilePath $executable -WorkingDirectory (Split-Path $executable -Parent) -WindowStyle $style -PassThru
    $null=$process.WaitForExit(3000)
    $process.Refresh()
    if(-not $process.HasExited){
        $modules=@($process.Modules|ForEach-Object {[ordered]@{name=$_.ModuleName;path=$_.FileName}})
    }
}catch{$failure=$_.Exception.Message}
$record=[ordered]@{
    createdUtc=[DateTime]::UtcNow.ToString('o');os=[Environment]::OSVersion.VersionString
    sourceHash=$expected;path=$executable;launchError=$failure;visibleRequested=[bool]$Visible
    processId=if($process){$process.Id}else{$null}
    exited=if($process){$process.HasExited}else{$null}
    exitCode=if($process -and $process.HasExited){$process.ExitCode}else{$null}
    mainWindowTitle=if($process -and -not $process.HasExited){$process.MainWindowTitle}else{$null}
    modules=$modules;registeredModules=$false;installedFonts=$false
}
$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'startup.json') -Encoding utf8
$record|ConvertTo-Json -Depth 3
Write-Output "Evidence: $run"
if($failure){throw 'Producer startup failed; evidence retained'}

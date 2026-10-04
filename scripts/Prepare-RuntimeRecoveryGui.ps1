[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
function Output([string]$name){$item=@($summary.outputs|Where-Object path -eq $name);if($item.Count -ne 1){throw 'Output missing'};$p=Join-Path (Split-Path $summaryPath -Parent) $name;if((Hash $p) -ne $item[0].sha256){throw 'Output mismatch'};return $p}
$coreExe=Output 'build/Release/producer_core_tests.exe';$exe=Output 'install/bin/Producer.exe'
$dir=Join-Path $repo ('work/acceptance/runtime-recovery-gui/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$core=Join-Path $dir 'core'
$process=Start-Process -FilePath $coreExe -ArgumentList @(('"'+$core+'"'),'--runtime-recovery-gui-prepare') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'prepare-stdout.txt') -RedirectStandardError (Join-Path $dir 'prepare-stderr.txt')
$timeout=-not $process.WaitForExit(15000);$exit=$null;if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}
$passed=-not $timeout -and $exit -eq 0;$journal=$null;$inputs=@();$outputs=@()
if($passed){
 $journal=[Text.Encoding]::Unicode.GetString([IO.File]::ReadAllBytes((Join-Path $core 'recovery-location.txt'))).TrimEnd([char]0)
 $inputs=@(Get-ChildItem -LiteralPath (Join-Path $core 'Source') -File -Recurse|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})+@([ordered]@{path=$journal;sha256=Hash $journal})
 $outputs=@(Get-ChildItem -LiteralPath (Join-Path $core 'Runtime') -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName;creationTimeUtc=$_.CreationTimeUtc.ToString('o');lastWriteTimeUtc=$_.LastWriteTimeUtc.ToString('o');attributes=[int]$_.Attributes}})
}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;coreExecutable=$coreExe;coreExeSha256=Hash $coreExe;productExecutable=$exe;productExeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;processId=$process.Id;exitCode=$exit;timedOut=$timeout;coreDirectory=$core;project=(Join-Path $core 'Source/Source.pro');journal=$journal;inputs=$inputs;publishedOutputs=$outputs;passed=$passed;scope='Fresh controlled incomplete rollback; both outputs existed before publication. GUI execution and restoration are separate, not inferred';fullAcceptance=$false}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dir 'prepare.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir);if(-not $passed){throw 'GUI fixture preparation failed; evidence retained without replay'}

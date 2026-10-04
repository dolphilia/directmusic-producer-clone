[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
$output=@($summary.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($output.Count -ne 1){throw 'Core output missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path;if((Hash $exe) -ne $output[0].sha256){throw 'Core EXE mismatch'}
$dir=Join-Path $repo ('work/acceptance/runtime-recovery-record/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$p=Start-Process -FilePath $exe -ArgumentList @(('"'+(Join-Path $dir 'core')+'"'),'--runtime-recovery-record') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;processId=$p.Id;exitCode=$exit;timedOut=$timeout;passed=($exit -eq 0 -and -not $timeout);scope='Recovery manifest serialization and invalid inputs; crash/replay/GUI/audio unexecuted';fullAcceptance=$false}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir)
if($timeout -or $exit -ne 0){throw 'Recovery record test failed; retain evidence'}

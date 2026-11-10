[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Successful unchanged source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/workspace source identity mismatch'}}
$identity=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($identity.Count -ne 1){throw 'Host output identity missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $identity[0].path;if((Hash $exe) -ne $identity[0].sha256){throw 'Host output hash mismatch'}
$run=Join-Path $repo ('work/acceptance/short-playback-monitor/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
$driverHash=Hash $PSCommandPath;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$arguments=@('--short-playback-monitor',('"'+(Join-Path $run 'short')+'"'));$exit=$null;$launchError=$null;$timeout=$false;$process=$null
try {$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'short-monitor.stdout.txt') -RedirectStandardError (Join-Path $run 'short-monitor.stderr.txt');$timeout=-not $process.WaitForExit(15000);if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}}
catch {$launchError=$_.Exception.Message}
$passed=$null -eq $launchError -and -not $timeout -and $exit -eq 0
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);driverSha256=$driverHash;driverCopy=(Join-Path $run 'driver.ps1');cases=@([ordered]@{name='short-monitor';executable=$exe;sha256=$identity[0].sha256;arguments=$arguments;exitCode=$exit;launchError=$launchError;timedOut=$timeout;processId=if($process){$process.Id}else{$null};passed=$passed});coreExecuted=$false;uiAcceptance='unexecuted';audioAcceptance='unexecuted';scope='Short notification monitor only; no whole native suite or prior success transferred';fullAcceptancePassed=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if(-not $passed){throw 'Targeted host smoke failed; evidence retained'}

[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $build.passed){throw 'Verified build required'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'build/Release/producer_core_tests.exe';$identity=@($build.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe')
if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Core EXE identity mismatch'}
$run=Join-Path $repo ('work/acceptance/playback-monitor/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$arguments=@(('"'+(Join-Path $run 'core')+'"'),'--playback-monitor')
$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'core.stdout.txt') -RedirectStandardError (Join-Path $run 'core.stderr.txt')
if(-not $process.WaitForExit(10000)){throw ('Core test remains live PID '+$process.Id)};$process.Refresh()
$proof=if($process.ExitCode -eq 0){Get-Content -LiteralPath (Join-Path $run 'core.stdout.txt') -Raw|ConvertFrom-Json}else{$null}
$passed=$process.ExitCode -eq 0 -and $proof.passed -and $proof.checks -eq 15
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;sourceCount=$build.sources.Count;driverSha256=Hash $PSCommandPath;processId=$process.Id;exitCode=$process.ExitCode;arguments=$arguments;checks=$proof.checks;passed=$passed;fullAcceptance=$false;scope='Per-instance history, peer natural end, long scheduled clock delay, grace and targeted timeout; no GUI/audio acceptance'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run);if(-not $passed){throw 'Playback monitor core failed'}

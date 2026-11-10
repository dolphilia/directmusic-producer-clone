[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261006T211007927Z/build-summary.json',[string]$Node='node')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $build.passed -or -not $build.sourceSnapshotUnchanged){throw 'Verified saved build required'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe'
$identity=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Producer identity mismatch'}
$recorderSummary=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$recorder=Get-Content -LiteralPath $recorderSummary -Raw|ConvertFrom-Json
if(-not $recorder.passed -or (Hash $recorder.executable) -ne $recorder.sha256){throw 'Recorder identity mismatch'}
foreach($s in $recorder.sources){if((Hash (Join-Path (Split-Path $recorderSummary -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source identity mismatch'}}
$run=Join-Path $repo ('work/acceptance/transport-position/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');profile='transport-position';buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;producer=$exe;producerSha256=Hash $exe;recorder=$recorder.executable;recorderSha256=$recorder.sha256;recorderBuildSummary=$recorderSummary;recorderBuildSummarySha256=Hash $recorderSummary;driverSha256=Hash $PSCommandPath;state='starting';playerPid=$null;playerExitCode=$null;capturePid=$null;captureExitCode=$null;passed=$false;fullAcceptance=$false}
function SaveRecord{$record|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8}
SaveRecord;Write-Output ('Evidence: '+$run)
$capture=Start-Process -FilePath $recorder.executable -ArgumentList @(('"'+$run+'"'),'40','--silent-keepalive') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'capture.stdout.txt') -RedirectStandardError (Join-Path $run 'capture.stderr.txt')
$record.capturePid=$capture.Id;SaveRecord
$limit=[DateTime]::UtcNow.AddSeconds(5)
while(-not (Test-Path -LiteralPath (Join-Path $run 'ready.json')) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
if(-not (Test-Path -LiteralPath (Join-Path $run 'ready.json'))){$record.state='recorder-not-ready';SaveRecord;throw 'Recorder not ready; inspect live processes before replay'}
Start-Sleep -Seconds 2
$args=@('--transport-position-observe',('"'+(Join-Path $run 'native')+'"'))
$record['arguments']=$args;$record['playerStartUtc']=[DateTime]::UtcNow.ToString('o')
$player=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'player.stdout.txt') -RedirectStandardError (Join-Path $run 'player.stderr.txt')
$record.playerPid=$player.Id;$record.state='running';SaveRecord
if(-not $player.WaitForExit(35000)){$record.state='player-timeout';SaveRecord;throw ('Player remains live PID '+$player.Id+'; inspect before replay')}
$player.Refresh();$record.playerExitCode=$player.ExitCode;$record['playerEndUtc']=[DateTime]::UtcNow.ToString('o');SaveRecord
if(-not $capture.WaitForExit(40000)){$record.state='recorder-timeout';SaveRecord;throw ('Recorder remains live PID '+$capture.Id+'; inspect before replay')}
$capture.Refresh();$record.captureExitCode=$capture.ExitCode;$record.state='exited';SaveRecord
if($player.ExitCode -ne 0 -or $capture.ExitCode -ne 0){throw 'Runtime/capture failed; evidence preserved'}
Copy-Item -LiteralPath (Join-Path $run 'run.json') -Destination (Join-Path $run 'run-at-audio-analysis.json')
& $Node (Join-Path $PSScriptRoot 'Inspect-TransportPositionAudio.mjs') $run *> (Join-Path $run 'analysis.stdout.txt')
if($LASTEXITCODE -ne 0){throw ('Transport PCM analysis failed; '+$run)}

$record['audioProofSha256']=Hash (Join-Path $run 'transport-position-audio-proof.json');$record.passed=$true;SaveRecord
Write-Output ('Passed: '+$run)

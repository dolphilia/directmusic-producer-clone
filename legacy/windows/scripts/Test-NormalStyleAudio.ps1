[CmdletBinding()]
param([Parameter(Mandatory)][string]$PreparationRun,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261004T015225991Z/build-summary.json',[string]$Node='C:/Users/dolph/Tools/node-v24.18.1-win-x64/node.exe')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent
$prepPath=[IO.Path]::GetFullPath($PreparationRun);$prep=Get-Content $prepPath -Raw|ConvertFrom-Json
$summaryPath=$prep.buildSummary;$b=Get-Content $summaryPath -Raw|ConvertFrom-Json
if(-not $prep.passed -or $prep.checks -ne 17 -or -not $b.passed -or -not $b.sourceSnapshotUnchanged -or (Hash $summaryPath) -ne $prep.buildSummarySha256){throw 'Same-build preparation required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$fixture=Join-Path (Split-Path $prepPath -Parent) 'core';$proofPath=Join-Path (Split-Path $prepPath -Parent) 'normal-style-dls-proof.json';$proof=Get-Content $proofPath -Raw|ConvertFrom-Json
if(-not $proof.passed -or $proof.runSha256 -ne (Hash $prepPath)){throw 'Independent fixture proof required'}
$inputs=@($proof.outputs|ForEach-Object {if((Hash $_.path) -ne $_.sha256){throw 'Fixture changed'};$_})
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$out=@($b.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($out.Count -ne 1 -or (Hash $exe) -ne $out[0].sha256){throw 'Product identity mismatch'}
$recSummary=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$rec=Get-Content $recSummary -Raw|ConvertFrom-Json
if(-not $rec.passed -or (Hash $rec.executable) -ne $rec.sha256){throw 'Recorder identity mismatch'}
foreach($s in $rec.sources){if((Hash (Join-Path (Split-Path $recSummary -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$run=Join-Path $repo ('work/acceptance/normal-style-audio/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item $PSCommandPath (Join-Path $run 'driver.ps1')
$capture=$null;$player=$null;$captureExit=$null;$playerExit=$null;$captureTimeout=$false;$playerTimeout=$false;$errorText=$null;$readyUtc=$null;$startUtc=$null
try{
 $capture=Start-Process -FilePath $rec.executable -ArgumentList @(('"'+$run+'"'),'26') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'capture.stdout.txt') -RedirectStandardError (Join-Path $run 'capture.stderr.txt')
 $limit=[DateTime]::UtcNow.AddSeconds(5)
 while(-not (Test-Path (Join-Path $run 'ready.json')) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
 if(-not (Test-Path (Join-Path $run 'ready.json'))){throw 'Recorder did not become ready'}
 $readyUtc=[DateTime]::UtcNow.ToString('o');Start-Sleep -Seconds 2;$startUtc=[DateTime]::UtcNow.ToString('o')
 $player=Start-Process -FilePath $exe -ArgumentList @('--normal-style-lifecycle',('"'+(Join-Path $run 'lifecycle')+'"'),('"'+(Join-Path $fixture 'Normal.dmpj')+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'player.stdout.txt') -RedirectStandardError (Join-Path $run 'player.stderr.txt')
 $playerTimeout=-not $player.WaitForExit(25000)
 if(-not $playerTimeout){$player.Refresh();$playerExit=$player.ExitCode}
}catch{$errorText=$_.Exception.Message}
if($capture){$captureTimeout=-not $capture.WaitForExit(35000);if(-not $captureTimeout){$capture.Refresh();$captureExit=$capture.ExitCode}}
$passed=$null -eq $errorText -and -not $captureTimeout -and -not $playerTimeout -and $captureExit -eq 0 -and $playerExit -eq 0
[ordered]@{schema=1;profile='normal-style-lifecycle';preparation=$prepPath;preparationSha256=Hash $prepPath;fixtureProof=$proofPath;fixtureProofSha256=Hash $proofPath;inputs=$inputs;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;producer=$exe;producerSha256=Hash $exe;recorder=$rec.executable;recorderSha256=Hash $rec.executable;recorderBuildSummary=$recSummary;recorderBuildSummarySha256=Hash $recSummary;driverSha256=Hash $PSCommandPath;readyUtc=$readyUtc;playerStartUtc=$startUtc;playerPid=if($player){$player.Id}else{$null};capturePid=if($capture){$capture.Id}else{$null};playerExitCode=$playerExit;captureExitCode=$captureExit;playerTimedOut=$playerTimeout;captureTimedOut=$captureTimeout;error=$errorText;passed=$passed;cases=@([ordered]@{name='audio-lifecycle';executable=$exe;sha256=Hash $exe;passed=$passed});scope='Source normal Style root DLS generated-note/lifecycle and tempo recording; GUI unexecuted';fullAcceptancePassed=$false}|ConvertTo-Json -Depth 8|Set-Content (Join-Path $run 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$run)
if(-not $passed){throw 'Player/capture failed; evidence retained; live timed-out processes not restarted'}
& $Node (Join-Path $PSScriptRoot 'Inspect-NormalStyleAudio.mjs') $run | Set-Content (Join-Path $run 'analysis.stdout.txt') -Encoding utf8
if($LASTEXITCODE -ne 0){throw 'Normal Style audio verification failed; evidence retained'}
& (Join-Path $PSScriptRoot 'Inspect-ProductModules.ps1') -RunPath (Join-Path $run 'run.json') -CaseName audio-lifecycle

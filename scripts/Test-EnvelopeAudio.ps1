[CmdletBinding()]
param([Parameter(Mandatory)][string]$PreparationRun,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261004T032918787Z/build-summary.json')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$prepPath=[IO.Path]::GetFullPath($PreparationRun);$prep=Get-Content -LiteralPath $prepPath -Raw|ConvertFrom-Json;$proofPath=Join-Path (Split-Path $prepPath -Parent) 'input-proof.json';$proof=Get-Content -LiteralPath $proofPath -Raw|ConvertFrom-Json
if(-not $prep.passed -or -not $proof.passed -or (Hash $prepPath) -ne $proof.preparationSha256 -or (Hash $prep.executable) -ne $prep.exeSha256){throw 'Verified current preparation required'}
foreach($o in $proof.outputs){if((Hash $o.path) -ne $o.sha256){throw 'Fixture modified'}}
$recPath=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$rec=Get-Content -LiteralPath $recPath -Raw|ConvertFrom-Json
if(-not $rec.passed -or (Hash $rec.executable) -ne $rec.sha256){throw 'Recorder identity mismatch'}
foreach($s in $rec.sources){if((Hash (Join-Path (Split-Path $recPath -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$base=Join-Path (Split-Path $prepPath -Parent) ('audio-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $base|Out-Null;Write-Output ('Evidence: '+$base)
foreach($variant in @('Fast','Slow')){
 $dir=Join-Path $base $variant;New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$fixture=Join-Path $dir 'input';New-Item -ItemType Directory -Path $fixture|Out-Null
 foreach($name in @('Envelope.sgp','Envelope.bnp','Envelope.dmpj')){Copy-Item -LiteralPath (Join-Path $prep.fixture $name) -Destination (Join-Path $fixture $name)}
 Copy-Item -LiteralPath (Join-Path $prep.fixture ($variant+'.dls')) -Destination (Join-Path $fixture 'owned.dls')
 $inputs=@(Get-ChildItem -LiteralPath $fixture -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
 $capture=Start-Process -FilePath $rec.executable -ArgumentList @(('"'+$dir+'"'),'26') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'capture.stdout.txt') -RedirectStandardError (Join-Path $dir 'capture.stderr.txt')
 $limit=[DateTime]::UtcNow.AddSeconds(5);while(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json')) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
 if(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json'))){throw ('Recorder not ready; inspect PID '+$capture.Id+' before retry')}
 Start-Sleep -Seconds 2
 $player=Start-Process -FilePath $prep.executable -ArgumentList @('--normal-style-lifecycle',('"'+(Join-Path $dir 'lifecycle')+'"'),('"'+(Join-Path $fixture 'Envelope.dmpj')+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'player.stdout.txt') -RedirectStandardError (Join-Path $dir 'player.stderr.txt')
 $pt=-not $player.WaitForExit(25000);$pe=$null;if(-not $pt){$player.Refresh();$pe=$player.ExitCode}
 $ct=-not $capture.WaitForExit(15000);$ce=$null;if(-not $ct){$capture.Refresh();$ce=$capture.ExitCode}
 $passed=-not $pt -and -not $ct -and $pe -eq 0 -and $ce -eq 0
 [ordered]@{schema=1;profile='envelope-attack-lifecycle';variant=$variant;preparation=$prepPath;preparationSha256=Hash $prepPath;inputProof=$proofPath;inputProofSha256=Hash $proofPath;inputs=$inputs;buildSummary=$prep.buildSummary;buildSummarySha256=$prep.buildSummarySha256;producer=$prep.executable;producerSha256=$prep.exeSha256;recorder=$rec.executable;recorderSha256=$rec.sha256;recorderBuildSummary=$recPath;recorderBuildSummarySha256=Hash $recPath;driverSha256=Hash $PSCommandPath;playerPid=$player.Id;capturePid=$capture.Id;playerExitCode=$pe;captureExitCode=$ce;playerTimedOut=$pt;captureTimedOut=$ct;passed=$passed;cases=@([ordered]@{name='audio-lifecycle';executable=$prep.executable;sha256=$prep.exeSha256;passed=$passed});fullAcceptancePassed=$false}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding utf8
 if(-not $passed){throw 'Player/capture failure preserved; do not restart live processes'}
 Write-Output ($variant+' player/capture exit0; PCM acceptance separate')
}

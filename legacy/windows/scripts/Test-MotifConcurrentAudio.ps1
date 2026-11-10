[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$FixtureDirectory,[string]$Node='node', [string]$RecorderBuildSummaryPath='work/build/audio-capture/20261003T102354462Z/build-summary.json')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $build.passed){throw 'Verified build required'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$identity=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'EXE identity mismatch'}
$fixture=[IO.Path]::GetFullPath($FixtureDirectory);$manifest=Get-Content -LiteralPath (Join-Path $fixture 'fixture.json') -Raw|ConvertFrom-Json
$inputs=@($manifest.files|ForEach-Object {$p=Join-Path $fixture $_.file;if((Hash $p) -ne $_.sha256){throw 'Fixture identity mismatch'};[ordered]@{path=$p;sha256=Hash $p}})
$recSummary=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$rec=Get-Content -LiteralPath $recSummary -Raw|ConvertFrom-Json
if(-not $rec.passed -or (Hash $rec.executable) -ne $rec.sha256){throw 'Recorder identity mismatch'}
foreach($s in $rec.sources){if((Hash (Join-Path (Split-Path $recSummary -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$run=Join-Path $repo ('work/acceptance/audio-concurrent/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$capture=Start-Process -FilePath $rec.executable -ArgumentList @(('"'+$run+'"'),'24') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'capture.stdout.txt') -RedirectStandardError (Join-Path $run 'capture.stderr.txt')
$ready=Join-Path $run 'ready.json';$limit=[DateTime]::UtcNow.AddSeconds(5);while(-not (Test-Path -LiteralPath $ready) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50};if(-not(Test-Path -LiteralPath $ready)){throw ('Recorder not ready; inspect PID '+$capture.Id)};Start-Sleep -Seconds 2
$args=@('--motif-concurrent-audio',('"'+(Join-Path $run 'concurrent')+'"'),('"'+(Join-Path $fixture 'primary.stp')+'"'),('"'+(Join-Path $fixture 'secondary.stp')+'"'),'"Authored Motif"')
$player=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'player.stdout.txt') -RedirectStandardError (Join-Path $run 'player.stderr.txt')
if(-not $player.WaitForExit(25000)){throw ('Player remains live PID '+$player.Id+'; inspect before replay')};$player.Refresh();if(-not $capture.WaitForExit(10000)){throw ('Recorder remains live PID '+$capture.Id)};$capture.Refresh()
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;producer=$exe;producerSha256=Hash $exe;recorder=$rec.executable;recorderSha256=Hash $rec.executable;recorderBuildSummary=$recSummary;recorderBuildSummarySha256=Hash $recSummary;fixture=$fixture;fixtureSha256=Hash (Join-Path $fixture 'fixture.json');inputs=$inputs;playerPid=$player.Id;playerExitCode=$player.ExitCode;capturePid=$capture.Id;captureExitCode=$capture.ExitCode;driverSha256=Hash $PSCommandPath;arguments=$args;cases=@([ordered]@{name='audio-concurrent';executable=$exe;sha256=Hash $exe;passed=($player.ExitCode -eq 0)});fullAcceptance=$false;scope='Distinct DLS tones in shared Performance, individual Stop and all Stop; endpoint only'}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run);if($player.ExitCode -ne 0 -or $capture.ExitCode -ne 0){throw 'Player/capture failed; evidence preserved'}
& $Node (Join-Path $PSScriptRoot 'Inspect-MotifConcurrentAudio.mjs') $run *> (Join-Path $run 'analysis.stdout.txt');if($LASTEXITCODE -ne 0){throw ('Concurrent audio failed; '+$run)}
& (Join-Path $PSScriptRoot 'Inspect-ProductModules.ps1') -RunPath (Join-Path $run 'run.json') -CaseName audio-concurrent

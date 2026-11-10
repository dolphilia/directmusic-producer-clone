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
$run=Join-Path $repo ('work/acceptance/boundary-runtime/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$args=@('--boundary-runtime-observe',('"'+(Join-Path $run 'concurrent')+'"'),('"'+(Join-Path $fixture 'primary.stp')+'"'),('"'+(Join-Path $fixture 'secondary.stp')+'"'),'"Authored Motif"')
$player=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'player.stdout.txt') -RedirectStandardError (Join-Path $run 'player.stderr.txt')
if(-not $player.WaitForExit(45000)){throw ('Player remains live PID '+$player.Id+'; inspect before replay')};$player.Refresh()
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;producer=$exe;producerSha256=Hash $exe;fixture=$fixture;fixtureSha256=Hash (Join-Path $fixture 'fixture.json');inputs=$inputs;playerPid=$player.Id;playerExitCode=$player.ExitCode;driverSha256=Hash $PSCommandPath;arguments=$args;cases=@([ordered]@{name='boundary-runtime';executable=$exe;sha256=Hash $exe;passed=($player.ExitCode -eq 0)});fullAcceptance=$false;scope='Secondary Immediate/Grid/Beat/Measure/Stored default resolution and primary alignment; no audio or GUI'}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run);if($player.ExitCode -ne 0){throw 'Player/capture failed; evidence preserved'}

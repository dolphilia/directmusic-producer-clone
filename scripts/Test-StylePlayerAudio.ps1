[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$FixtureDirectory,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261006T211007927Z/build-summary.json')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Successful immutable build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Candidate source mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$o=@($b.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($o.Count -ne 1 -or (Hash $exe) -ne $o[0].sha256){throw 'Product identity mismatch'}
$fixture=[IO.Path]::GetFullPath($FixtureDirectory);$f=Get-Content "$fixture/fixture.json" -Raw|ConvertFrom-Json
$inputs=@($f.files|ForEach-Object {$p=Join-Path $fixture $_.file;if((Hash $p) -ne $_.sha256){throw 'Fixture mismatch'};[ordered]@{path=$p;sha256=Hash $p}})
$recPath=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$rec=Get-Content $recPath -Raw|ConvertFrom-Json;if(-not $rec.passed -or (Hash $rec.executable) -ne $rec.sha256){throw 'Recorder identity mismatch'}
foreach($s in $rec.sources){if((Hash (Join-Path (Split-Path $recPath -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$dir=Join-Path $repo ('work/acceptance/style-player-audio/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item $PSCommandPath "$dir/driver.ps1"
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');candidate=Split-Path (Split-Path $summaryPath -Parent) -Leaf;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;recorder=$rec.executable;recorderSha256=Hash $rec.executable;recorderBuildSummary=$recPath;recorderBuildSummarySha256=Hash $recPath;fixture=$fixture;fixtureSha256=Hash "$fixture/fixture.json";inputs=$inputs;driverSha256=Hash $PSCommandPath;playerPid=$null;playerExitCode=$null;capturePid=$null;captureExitCode=$null;error=$null;passed=$false;scope='Source StylePlayer dedicated lifecycle PCM; main GUI/save/reload and original parity separate';fullAcceptance=$false}
try{
 $capture=Start-Process -FilePath $rec.executable -ArgumentList @(('"'+$dir+'"'),'46','--silent-keepalive') -WindowStyle Hidden -PassThru -RedirectStandardOutput "$dir/capture.stdout.txt" -RedirectStandardError "$dir/capture.stderr.txt";$record.capturePid=$capture.Id
 $deadline=[DateTime]::UtcNow.AddSeconds(5);while(-not(Test-Path "$dir/ready.json") -and [DateTime]::UtcNow -lt $deadline -and -not $capture.HasExited){Start-Sleep -Milliseconds 50};if(-not(Test-Path "$dir/ready.json")){throw 'Recorder not ready; no replay'};Start-Sleep -Seconds 2
 $args=@('--style-player-audio',('"'+$dir+'/player"'),('"'+$fixture+'/Shape.stp"'),('"'+$fixture+'/ShapeMap.cdm"'));$record['arguments']=$args
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$dir/player.stdout.txt" -RedirectStandardError "$dir/player.stderr.txt";$record.playerPid=$p.Id
 if(-not $p.WaitForExit(42000)){throw 'Player timeout; retained without retry'};$p.Refresh();$record.playerExitCode=$p.ExitCode
 if(-not $capture.WaitForExit(15000)){throw 'Recorder still live; no replay'};$capture.Refresh();$record.captureExitCode=$capture.ExitCode
 $record.passed=$p.ExitCode -eq 0 -and $capture.ExitCode -eq 0
 foreach($i in $inputs){if((Hash $i.path) -ne $i.sha256){throw 'Input changed'}}
}catch{$record.error=$_.Exception.Message;$record.passed=$false}
$record|ConvertTo-Json -Depth 7|Set-Content "$dir/run.json" -Encoding utf8;Write-Output ('Evidence: '+$dir)
if(-not $record.passed){throw 'StylePlayer runtime/capture failed; preserved'}
& node "$PSScriptRoot/Inspect-StylePlayerAudio.mjs" $dir *> "$dir/analysis.stdout.txt";if($LASTEXITCODE -ne 0){throw 'StylePlayer PCM audit failed; preserved'}

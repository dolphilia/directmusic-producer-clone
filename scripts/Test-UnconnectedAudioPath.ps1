[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$FixtureRoot,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261004T032918787Z/build-summary.json')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent
$bs=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $bs -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Successful unchanged source build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Current/saved source mismatch'}}
$exe=Join-Path (Split-Path $bs -Parent) 'install/bin/Producer.exe';$output=@($b.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($output.Count -ne 1 -or (Hash $exe) -ne $output[0].sha256){throw 'Producer identity mismatch'}
$rs=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$rec=Get-Content -LiteralPath $rs -Raw|ConvertFrom-Json
if(-not $rec.passed -or (Hash $rec.executable) -ne $rec.sha256){throw 'Recorder identity mismatch'}
foreach($s in $rec.sources){if((Hash (Join-Path (Split-Path $rs -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$fixture=[IO.Path]::GetFullPath($FixtureRoot);$proofPath=Join-Path $fixture 'fixture-proof.json';$proof=Get-Content -LiteralPath $proofPath -Raw|ConvertFrom-Json
$base=Join-Path $repo ('work/acceptance/unconnected-audiopath/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $base|Out-Null
Write-Output ('Evidence: '+$base)
foreach($variant in @('all-unmapped','mixed')){
 $source=Join-Path $fixture $variant;$expected=@($proof.outputs|Where-Object variant -eq $variant)
 if($expected.Count -ne 1){throw 'Fixture variant missing'}
 foreach($f in $expected[0].files){if((Hash (Join-Path $source $f.name)) -ne $f.sha256){throw 'Fixture changed'}}
 $inputs=@($expected[0].files|ForEach-Object {[ordered]@{path=Join-Path $source $_.name;sha256=$_.sha256}})
 $dir=Join-Path $base $variant;New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
 $record=[ordered]@{schema=1;candidate=(Split-Path (Split-Path $bs -Parent) -Leaf);variant=$variant;buildSummary=$bs;buildSummarySha256=Hash $bs;producer=$exe;producerSha256=Hash $exe;recorder=$rec.executable;recorderSha256=$rec.sha256;recorderBuildSummary=$rs;recorderBuildSummarySha256=Hash $rs;driverSha256=Hash $PSCommandPath;inputProof=$proofPath;inputProofSha256=Hash $proofPath;inputs=$inputs;playerExitCode=$null;captureExitCode=$null;passed=$false;fullAcceptance=$false}
 function Save {$record|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding utf8}
 $capture=Start-Process -FilePath $rec.executable -ArgumentList @(('"'+$dir+'"'),'26') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'capture.stdout.txt') -RedirectStandardError (Join-Path $dir 'capture.stderr.txt')
 $record['capturePid']=$capture.Id;Save
 $deadline=[DateTime]::UtcNow.AddSeconds(5)
 while(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json')) -and [DateTime]::UtcNow -lt $deadline -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
 if(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json'))){Save;throw 'Recorder not ready; no retry'}
 Start-Sleep -Seconds 2
 $player=Start-Process -FilePath $exe -ArgumentList @('--audio-lifecycle',('"'+(Join-Path $dir 'lifecycle')+'"'),('"'+(Join-Path $source 'Test.pro')+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'player.stdout.txt') -RedirectStandardError (Join-Path $dir 'player.stderr.txt')
 $record['playerPid']=$player.Id;Save
 $pt=-not $player.WaitForExit(25000);if(-not $pt){$player.Refresh();$record.playerExitCode=$player.ExitCode};$record['playerTimedOut']=$pt;Save
 $ct=-not $capture.WaitForExit(30000);if(-not $ct){$capture.Refresh();$record.captureExitCode=$capture.ExitCode};$record['captureTimedOut']=$ct
 $record['inputsUnchanged']=@($inputs|Where-Object {(Hash $_.path) -ne $_.sha256}).Count -eq 0
 $record.passed=-not $pt -and -not $ct -and $record.playerExitCode -eq 0 -and $record.captureExitCode -eq 0 -and $record.inputsUnchanged;Save
 if(-not $record.passed){throw ('Retained '+$variant+' lifecycle failure; inspect before another run')}
 Write-Output ($variant+' lifecycle/capture exit0; PCM verdict separate')
}

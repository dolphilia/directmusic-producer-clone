[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $build.passed -or -not $build.sourceSnapshotUnchanged){throw 'Verified saved build required'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$identity=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Producer identity mismatch'}
$run=Join-Path $repo ('work/acceptance/midi-echo/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$arguments=@('--midi-echo-observe',('"'+(Join-Path $run 'native')+'"'))
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');profile='midi-echo';buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;arguments=$arguments;processId=$null;processStartUtc=$null;exitCode=$null;state='starting';passed=$false;fullAcceptance=$false}
function SaveRecord{$record|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8}
SaveRecord;Write-Output ('Evidence: '+$run)
$player=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
$record.processId=$player.Id;$record.processStartUtc=$player.StartTime.ToUniversalTime().ToString('o');$record.state='running';SaveRecord
if(-not $player.WaitForExit(35000)){$record.state='timeout-process-retained';SaveRecord;throw 'Runtime remains live; inspect before replay'}
$player.Refresh();$record.exitCode=$player.ExitCode;$record.state='exited';SaveRecord
$native=Join-Path $run 'native/midi-echo.json';if(Test-Path -LiteralPath $native){$result=Get-Content -LiteralPath $native -Raw|ConvertFrom-Json;$record['nativeSha256']=Hash $native;$record.passed=$player.ExitCode -eq 0 -and $result.passed;SaveRecord}
if(-not $record.passed){throw 'MIDI Echo routing verification failed; evidence preserved'}
Write-Output ('Passed: '+$run)

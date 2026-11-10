[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$AudioPath='work/producer/samples/Tutorial/FinishedProject/APFarm.aup',[string]$Node='C:/Users/dolph/Tools/node-v24.18.1-win-x64/node.exe')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summary=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summary -Raw|ConvertFrom-Json
if(-not $build.passed -or -not $build.sourceSnapshotUnchanged){throw 'Successful unchanged build required'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$exe=Join-Path (Split-Path $summary -Parent) 'install/bin/Producer.exe';$output=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($output.Count -ne 1 -or (Hash $exe) -ne $output[0].sha256){throw 'Product identity mismatch'}
$dir=Join-Path $repo ('work/acceptance/runtime-settings-input/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$tone=Join-Path $dir 'tone.wav'
& $Node (Join-Path $PSScriptRoot 'Create-EnvelopeTone.mjs') $tone
if($LASTEXITCODE -ne 0){throw 'Tone generator failed'}
$audioInput=[IO.Path]::GetFullPath($AudioPath);$fixture=Join-Path $dir 'fixture';$process=Start-Process -FilePath $exe -ArgumentList @('--prepare-runtime-settings',('"'+$fixture+'"'),('"'+$tone+'"'),('"'+$audioInput+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $process.WaitForExit(15000);$exit=$null;if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}
[ordered]@{schema=1;runtimeSettings=$true;audioPath=$audioInput;audioPathSha256=Hash $audioInput;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summary;buildSummarySha256=Hash $summary;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;tone=$tone;toneSha256=Hash $tone;generatorSha256=Hash (Join-Path $PSScriptRoot 'Create-EnvelopeTone.mjs');fixture=$fixture;processId=$process.Id;exitCode=$exit;timedOut=$timeout;passed=($exit -eq 0 -and -not $timeout);fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$dir);if($timeout -or $exit -ne 0){throw 'Preparation failed; preserve evidence and inspect live process before retry'}

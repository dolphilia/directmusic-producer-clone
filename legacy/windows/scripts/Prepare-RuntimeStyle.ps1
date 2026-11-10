[CmdletBinding()]
param([Parameter(Mandatory)][string]$PreparationRun)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$prepPath=[IO.Path]::GetFullPath($PreparationRun);$prep=Get-Content -LiteralPath $prepPath -Raw|ConvertFrom-Json;$proofPath=Join-Path (Split-Path $prepPath -Parent) 'normal-style-dls-proof.json';$proof=Get-Content -LiteralPath $proofPath -Raw|ConvertFrom-Json
if(-not $prep.passed -or $prep.checks -ne 17 -or -not $proof.passed -or $proof.runSha256 -ne (Hash $prepPath)){throw 'Verified normal Style fixture required'}
$summaryPath=$prep.buildSummary;$build=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $build.passed -or (Hash $summaryPath) -ne $prep.buildSummarySha256){throw 'Build identity mismatch'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Current source mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$output=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($output.Count -ne 1 -or (Hash $exe) -ne $output[0].sha256){throw 'Product hash mismatch'}
$inputs=@($proof.outputs|ForEach-Object {if((Hash $_.path) -ne $_.sha256){throw 'Normal Style fixture changed'};$_})
$project=Join-Path (Split-Path $prepPath -Parent) 'core/Normal.dmpj';$dir=Join-Path $repo ('work/acceptance/runtime-style-input/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$fixture=Join-Path $dir 'fixture'
$process=Start-Process -FilePath $exe -ArgumentList @('--prepare-runtime-style',('"'+$fixture+'"'),('"'+$project+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt');$timeout=-not $process.WaitForExit(15000);$exit=$null;if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}
$unchanged=@($inputs|Where-Object {(Hash $_.path) -ne $_.sha256}).Count -eq 0
[ordered]@{schema=1;runtimeStyle=$true;createdUtc=[DateTime]::UtcNow.ToString('o');preparation=$prepPath;preparationSha256=Hash $prepPath;preparationProof=$proofPath;preparationProofSha256=Hash $proofPath;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputs=$inputs;project=$project;fixture=$fixture;processId=$process.Id;exitCode=$exit;timedOut=$timeout;inputBytesUnchanged=$unchanged;passed=($exit -eq 0 -and -not $timeout -and $unchanged);fullAcceptance=$false}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir);if($timeout -or $exit -ne 0 -or -not $unchanged){throw 'Runtime Style preparation failed; retain evidence without unchanged replay'}

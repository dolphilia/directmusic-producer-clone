[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$InputCollection)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Successful unchanged build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$output=@($b.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($output.Count -ne 1 -or (Hash $exe) -ne $output[0].sha256){throw 'Producer identity mismatch'}
$inputPath=[IO.Path]::GetFullPath($InputCollection);$dir=Join-Path $repo ('work/acceptance/authored-dls-lifecycle/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$inputHash=Hash $inputPath;$fixture=Join-Path $dir 'AuthoredDls';$p=Start-Process -FilePath $exe -ArgumentList @('--prepare-authored-dls',('"'+$fixture+'"'),('"'+$inputPath+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$inputs=@([ordered]@{path=$inputPath;sha256=$inputHash});$outputs=@();if(-not $timeout -and $exit -eq 0){$outputs=@(Get-ChildItem -LiteralPath $fixture -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');build=$summaryPath;buildSha256=Hash $summaryPath;executable=$exe;sha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputs=$inputs;inputUnchanged=((Hash $inputPath) -eq $inputHash);outputs=$outputs;exitCode=$exit;timedOut=$timeout;passed=($exit -eq 0 -and -not $timeout);scope='Typed loop/history, owned Band/Segment/native Project reload/resave only; no GUI restart/audio';fullAcceptance=$false}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir);if($timeout -or $exit -ne 0){throw 'Preparation failed; inspect preserved evidence'}

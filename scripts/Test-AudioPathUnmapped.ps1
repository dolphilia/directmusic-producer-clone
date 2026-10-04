[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Project)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Successful unchanged build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$output=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($output.Count -ne 1 -or (Hash $exe) -ne $output[0].sha256){throw 'Exe mismatch'}
$input=[IO.Path]::GetFullPath($Project);$inputs=@(Get-ChildItem (Split-Path $input -Parent) -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
$dir=Join-Path $repo ('work/acceptance/audiopath-unmapped/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory $dir|Out-Null;Copy-Item $PSCommandPath (Join-Path $dir 'driver.ps1')
$p=Start-Process -FilePath $exe -ArgumentList @('--normal-style-lifecycle',('"'+(Join-Path $dir 'lifecycle')+'"'),('"'+$input+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$native=Get-Content (Join-Path $dir 'lifecycle/lifecycle.json') -Raw|ConvertFrom-Json
$unmapped=@($native.calls|Where-Object {$_.operation -eq 'Convert embedded AudioPath PChannel' -and $_.hresult -eq 2289570145})
$forbidden=@($native.calls|Where-Object {$_.operation -eq 'Download segment' -or $_.operation -eq 'PlaySegmentEx'})
$closed=@($native.calls|Where-Object {$_.operation -eq 'CloseDown' -and $_.hresult -eq 0})
$unchanged=$true;foreach($x in $inputs){if((Hash $x.path) -ne $x.sha256){$unchanged=$false}}
$passed=-not $timeout -and $exit -eq 1 -and -not $native.passed -and $unmapped.Count -eq 1 -and $forbidden.Count -eq 0 -and $closed.Count -eq 1 -and $unchanged
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');passed=$passed;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;project=$input;inputs=$inputs;inputsUnchanged=$unchanged;pid=$p.Id;exitCode=$exit;timedOut=$timeout;nativeSha256=Hash (Join-Path $dir 'lifecycle/lifecycle.json');driverSha256=Hash $PSCommandPath;scope='PChannel0 absent in preserved APFarm routes; explicit ConvertPChannel rejection before Download/Play, cleanup and unchanged inputs. No recording required or audio success claimed.';fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content (Join-Path $dir 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$dir);if(-not $passed){throw 'Unmapped rejection/cleanup failed; evidence preserved'}

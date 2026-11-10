[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$Project='work/producer/samples/QuickStart/QuickStart.pro')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
$output=@($summary.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($output.Count -ne 1){throw 'Core output missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path;if((Hash $exe) -ne $output[0].sha256){throw 'Core EXE mismatch'}
$inputProject=[IO.Path]::GetFullPath($Project);$inputs=@(Get-ChildItem -LiteralPath (Split-Path $inputProject -Parent) -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
$dir=Join-Path $repo ('work/acceptance/jazp-save/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$arguments=@(('"'+(Join-Path $dir 'core')+'"'),'--jazp-save',('"'+$inputProject+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$record=[ordered]@{schema=2;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputs=$inputs;project=$inputProject;processId=$p.Id;exitCode=$exit;timedOut=$timeout;passed=($exit -eq 0 -and -not $timeout);scope='Targeted empty/new Segment/Style/Band/mixed native factory, existing-entry persistence and native sharing-lock atomicity; full core/GUI/original application reopen/audio unexecuted';fullAcceptance=$false}
$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8;Write-Output ('Evidence: '+$dir)
if(-not $record.passed){throw 'JAZP save failed; preserve evidence and inspect process before replay'}

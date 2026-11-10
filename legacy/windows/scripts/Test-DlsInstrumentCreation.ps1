[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$Observation='work/analysis/dls-instrument-original/20261003T231831Z')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
$output=@($summary.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($output.Count -ne 1){throw 'Core output missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path;if((Hash $exe) -ne $output[0].sha256){throw 'Core EXE mismatch'}
$observationPath=[IO.Path]::GetFullPath($Observation);$wav=Join-Path $repo 'work/analysis/dls-author-original/20261003T224300Z/Tone.wav';$dls=Join-Path $observationPath 'before.dlp'
$inputs=@($wav,$dls|ForEach-Object {[ordered]@{path=$_;sha256=Hash $_}})
$dir=Join-Path $repo ('work/acceptance/dls-instrument-creation/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$arguments=@(('"'+(Join-Path $dir 'core')+'"'),'--dls-create-instrument',('"'+$wav+'"'),('"'+$dls+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputs=$inputs;processId=$p.Id;exitCode=$exit;timedOut=$timeout;passed=($exit -eq 0 -and -not $timeout);scope='Explicit Instrument and Region creation, input rejection, history, native reload, Band assignment, alias cue and opaque preservation; original saved defaults/GUI/audio/full acceptance unexecuted';fullAcceptance=$false}
$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8;Write-Output ('Evidence: '+$dir)
if(-not $record.passed){throw 'DLS Instrument creation failed; preserve evidence and inspect before replay'}

[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$Dls='work/acceptance/normal-style-dls/20261004T025021534Z/core/owned.dls',[string]$Original='work/analysis/dls-instrument-original/20261003T231831Z/after-instrument.dlp')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Verified build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash $s.path) -ne $s.sha256){throw 'Source identity mismatch'}}
$o=@($b.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($o.Count -ne 1){throw 'Core identity missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $o[0].path;if((Hash $exe) -ne $o[0].sha256){throw 'Core identity mismatch'}
$inputs=@($Dls,$Original)|ForEach-Object {$p=[IO.Path]::GetFullPath($_);[ordered]@{path=$p;sha256=Hash $p}}
$dir=[IO.Path]::GetFullPath('work/acceptance/dls-articulation/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$p=Start-Process -FilePath $exe -ArgumentList @(('"'+(Join-Path $dir 'core')+'"'),'--dls-articulation',('"'+$inputs[0].path+'"'),('"'+$inputs[1].path+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$passed=$exit -eq 0 -and -not $timeout;foreach($i in $inputs){if((Hash $i.path) -ne $i.sha256){$passed=$false}}
$checks=$null;if($passed){$native=Get-Content -LiteralPath (Join-Path $dir 'stdout.txt') -Raw|ConvertFrom-Json;$checks=$native.checks;$passed=$native.passed}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputs=$inputs;processId=$p.Id;exitCode=$exit;timedOut=$timeout;passed=$passed;checks=$checks;scope='Typed owned articulation edit/history/native project reload; original saved connection fixture comparison; GUI/runtime units/audio/full acceptance unexecuted';fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir);if(-not $passed){throw 'Articulation test failed; inspect same process/evidence before retry'}

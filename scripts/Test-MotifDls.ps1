[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Dls)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Verified build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$o=@($b.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($o.Count -ne 1){throw 'Core identity missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $o[0].path;if((Hash $exe) -ne $o[0].sha256){throw 'Core identity mismatch'}
$inputPath=[IO.Path]::GetFullPath($Dls);$inputHash=Hash $inputPath
$dir=Join-Path $repo ('work/acceptance/motif-dls/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
Copy-Item -LiteralPath $inputPath -Destination (Join-Path $dir 'input.dls')
$exit=$null;$launchError=$null;$timeout=$false;$p=$null
try{$p=Start-Process -FilePath $exe -ArgumentList @(('"'+(Join-Path $dir 'core')+'"'),'--motif-dls',('"'+$inputPath+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt');$timeout=-not $p.WaitForExit(15000);if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}}catch{$launchError=$_.Exception.Message}
$passed=$exit -eq 0 -and -not $timeout -and -not $launchError;$checks=$null;if($passed){$n=Get-Content -LiteralPath (Join-Path $dir 'stdout.txt') -Raw|ConvertFrom-Json;$passed=$n.passed -and $n.scope -eq 'Motif DLS assignment/cache fixture; runtime unverified';$checks=$n.checks}
if((Hash $inputPath) -ne $inputHash){$passed=$false}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;executableSha256=Hash $exe;driverSha256=Hash $PSCommandPath;inputPath=$inputPath;inputSha256=$inputHash;passed=$passed;checks=$checks;exitCode=$exit;launchError=$launchError;timedOut=$timeout;processId=if($p){$p.Id}else{$null};scope='Motif DLS assignment and owned cache/history/save/reload; runtime GUI audio unverified';fullAcceptance=$false}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir);if(-not $passed){throw 'Motif DLS native failed; evidence retained'}

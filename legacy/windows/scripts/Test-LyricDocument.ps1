[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Successful unchanged source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/workspace source identity mismatch'}}
$identity=@($summary.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');if($identity.Count -ne 1){throw 'Core executable identity missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $identity[0].path;if((Hash $exe) -ne $identity[0].sha256){throw 'Core executable hash mismatch'}
$run=Join-Path $repo ('work/acceptance/lyric-document/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
$driverHash=Hash $PSCommandPath;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$inputRoot=Join-Path $repo 'work/analysis/q3-lyric/20261004T212600Z'
$inputs=@(Get-ChildItem -LiteralPath $inputRoot -File -Recurse|Sort-Object FullName|ForEach-Object {[ordered]@{path=$_.FullName;sha256=(Hash $_.FullName)}})
$arguments=@(('"'+(Join-Path $run 'core')+'"'),'--lyric-document',('"'+$inputRoot+'"'));$exit=$null;$launchError=$null;$timeout=$false;$process=$null
try {$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt');$timeout=-not $process.WaitForExit(15000);if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}}
catch {$launchError=$_.Exception.Message}
$unchanged=@($inputs|Where-Object {(Hash $_.path) -ne $_.sha256}).Count -eq 0
$passed=$unchanged -and $null -eq $launchError -and -not $timeout -and $exit -eq 0
$checks=$null;if($passed){$result=Get-Content -LiteralPath (Join-Path $run 'stdout.txt') -Raw|ConvertFrom-Json;$passed=$result.passed -and $result.scope -eq 'owned Lyric Unicode clocks delivery CRUD clipboard history original inputs native Project; GUI tools playback separate';$checks=$result.checks}
$files=@();$fixtureDirectory=Join-Path $run 'core/lyric-document';if(Test-Path -LiteralPath $fixtureDirectory){$files=@(Get-ChildItem -LiteralPath $fixtureDirectory -File -Recurse | ForEach-Object {[ordered]@{name=[IO.Path]::GetRelativePath($fixtureDirectory,$_.FullName);sha256=(Hash $_.FullName);bytes=$_.Length}})}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);executable=$exe;executableSha256=$identity[0].sha256;driverSha256=$driverHash;arguments=$arguments;exitCode=$exit;launchError=$launchError;timedOut=$timeout;processId=if($process){$process.Id}else{$null};passed=$passed;checks=$checks;files=$files;inputs=$inputs;inputsUnchanged=$unchanged;stdoutSha256=if(-not $timeout -and (Test-Path -LiteralPath (Join-Path $run 'stdout.txt'))){Hash (Join-Path $run 'stdout.txt')}else{$null};scope='Only targeted Lyric CRUD native checks; standard host/core regression, GUI, audio, original comparison unexecuted for this build';fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if(-not $passed){throw 'Targeted Lyric CRUD failed; evidence retained'}

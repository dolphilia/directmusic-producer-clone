[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Segment,[string[]]$InputPaths=@(),[string]$MotifName='',[switch]$StandaloneStyle,[ValidateRange(1000,120000)][int]$ObservationMs=12000)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified successful build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256){throw 'Saved source identity mismatch'}}
$inputPath=[IO.Path]::GetFullPath($Segment);if(-not (Test-Path -LiteralPath $inputPath -PathType Leaf)){throw 'Group input missing; no launch'}
$identity=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($identity.Count -ne 1){throw 'Installed product identity missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $identity[0].path;if((Hash $exe) -ne $identity[0].sha256){throw 'Installed product identity mismatch'}
$run=Join-Path $repo ('work/acceptance/product-notes/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
$driverHash=Hash $PSCommandPath;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'runtime-driver.ps1')
$inputHash=Hash $inputPath;$arguments=@('--note-observe',('"'+(Join-Path $run 'notes')+'"'),('"'+$inputPath+'"'))
if($MotifName){if($MotifName.Contains('"') -or $MotifName.Contains([char]0)){throw 'Unsupported command-line Motif name'};$arguments[0]='--motif-observe';$arguments+=('"'+$MotifName+'"')}
if($StandaloneStyle){if(-not $MotifName){throw 'Standalone Style requires an explicit Motif'};$arguments[0]='--style-motif-observe'}
if(-not $MotifName){$arguments+=([string]$ObservationMs)}elseif($ObservationMs -ne 12000){throw 'Custom observation window is available for Segment note-observe only'}
$dependencies=@($InputPaths|ForEach-Object {$p=[IO.Path]::GetFullPath($_);[ordered]@{path=$p;sha256=(Hash $p)}})
$exit=$null;$launchError=$null;$nativeError=$null;$timeout=$false
try {$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'notes-api.stdout.txt') -RedirectStandardError (Join-Path $run 'notes-api.stderr.txt');$timeout=-not $process.WaitForExit($ObservationMs+6000);if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}}
catch {$launchError=$_.Exception.Message;$inner=$_.Exception;while($inner.InnerException){$inner=$inner.InnerException};if($inner -is [ComponentModel.Win32Exception]){$nativeError=$inner.NativeErrorCode}}
$passed=$null -eq $launchError -and -not $timeout -and $exit -eq 0
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);runtimeDriverSha256=$driverHash;runtimeDriverCopy=(Join-Path $run 'runtime-driver.ps1');observationMs=$ObservationMs;groupPlaybackInput=[ordered]@{path=$inputPath;sha256=$inputHash};dependencyInputs=$dependencies;cases=@([ordered]@{name='notes-api';executable=$exe;sha256=$identity[0].sha256;arguments=$arguments;exitCode=$exit;launchError=$launchError;nativeErrorCode=$nativeError;timedOut=$timeout;passed=$passed});executionBlocked=($null -ne $launchError);coreExecuted=$false;uiAcceptance='unexecuted';audioAcceptance='unexecuted';fullAcceptancePassed=$false;scope='Targeted generated runtime note observation only; no previous host/core result transferred'}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if(-not $passed){throw 'Note observation failed; inspect evidence'}


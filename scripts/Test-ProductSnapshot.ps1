[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$ReferenceSegment,[string]$ReferenceStyleDirectory,[string]$ReferenceCollection,[switch]$Playback,[switch]$StylePlayback,[switch]$DlsPlayback,[string]$DlsPlaybackCollection,[ValidateRange(0,2147483647)][int]$DlsPlaybackInstrument=0,[switch]$DlsPlaybackCoreWave,[switch]$DlsPlaybackCorePcm,[switch]$DlsPlaybackCoreResize,[switch]$DlsPlaybackCoreRegion,[switch]$DlsPlaybackCoreSaveAs,[switch]$DlsPlaybackCoreDelete,[string]$GroupPlaybackSegment)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath)
$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified successful snapshot required'}
foreach($source in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $source.path)) -ne $source.sha256){throw 'Snapshot source changed'}}
if($ReferenceSegment -and -not (Test-Path -LiteralPath $ReferenceSegment -PathType Leaf)){throw 'Reference input missing; no executable launched'}
if($GroupPlaybackSegment -and -not (Test-Path -LiteralPath $GroupPlaybackSegment -PathType Leaf)){throw 'Group playback input missing; no executable launched'}
if($ReferenceStyleDirectory -and (-not $ReferenceSegment -or -not (Test-Path -LiteralPath $ReferenceStyleDirectory -PathType Container))){throw 'Style directory requires a reference segment and an existing directory'}
if($StylePlayback -and (-not $ReferenceStyleDirectory -or -not $ReferenceSegment)){throw 'Style playback requires explicit segment and Style directory'}
if($ReferenceCollection -and (-not $ReferenceStyleDirectory -or -not (Test-Path -LiteralPath $ReferenceCollection -PathType Leaf))){throw 'Collection validation requires an existing input and explicit Style/segment arguments'}
if($DlsPlayback -and -not $ReferenceCollection){throw 'DLS playback requires explicit collection input'}
if(($DlsPlaybackCoreWave -or $DlsPlaybackCorePcm -or $DlsPlaybackCoreResize -or $DlsPlaybackCoreRegion -or $DlsPlaybackCoreSaveAs -or $DlsPlaybackCoreDelete) -and (-not $DlsPlayback -or $DlsPlaybackCollection -or (@($DlsPlaybackCoreWave,$DlsPlaybackCorePcm,$DlsPlaybackCoreResize,$DlsPlaybackCoreRegion,$DlsPlaybackCoreSaveAs,$DlsPlaybackCoreDelete | Where-Object {$_}).Count -gt 1))){throw 'Core Wave playback requires DlsPlayback and no external playback collection'}
if(-not $DlsPlaybackCollection){$DlsPlaybackCollection=$ReferenceCollection}
if($DlsPlayback -and -not ($DlsPlaybackCoreWave -or $DlsPlaybackCorePcm -or $DlsPlaybackCoreResize -or $DlsPlaybackCoreRegion -or $DlsPlaybackCoreSaveAs -or $DlsPlaybackCoreDelete) -and -not (Test-Path -LiteralPath $DlsPlaybackCollection -PathType Leaf)){throw 'DLS playback collection missing'}
$run=Join-Path $repo ('work/acceptance/product/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run|Out-Null
if($DlsPlaybackCoreWave){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/wave-crud/source.dls'}
if($DlsPlaybackCorePcm){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/pcm-io/imported.dls'}
if($DlsPlaybackCoreResize){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/pcm-resize/grown.dls'}
if($DlsPlaybackCoreRegion){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/region-create/source.dls'}
if($DlsPlaybackCoreSaveAs){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/save-as/renamed.dls'}
if($DlsPlaybackCoreDelete){$DlsPlaybackCollection=Join-Path $run 'core/dls-editor/wave-remove/deleted.dls'}
$cases=@()
$blocked=$false
function Invoke-Case([string]$name,[string]$outputPath,[string[]]$arguments) {
  $root=Split-Path $summaryPath -Parent;$file=Join-Path $root $outputPath
  $identity=@($summary.outputs|Where-Object path -eq $outputPath)
  if($identity.Count -ne 1 -or (Hash $file) -ne $identity[0].sha256){throw 'Output identity mismatch'}
  $exit=$null;$launchError=$null;$code=$null;$timeout=$false
  try {
    $process=Start-Process -FilePath $file -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run ($name+'.stdout.txt')) -RedirectStandardError (Join-Path $run ($name+'.stderr.txt'))
    $timeout=-not $process.WaitForExit(15000);if($timeout){Stop-Process -Id $process.Id;$process.WaitForExit()};$process.Refresh();$exit=$process.ExitCode
  } catch {$launchError=$_.Exception.Message;$inner=$_.Exception;while($inner.InnerException){$inner=$inner.InnerException};if($inner -is [ComponentModel.Win32Exception]){$code=$inner.NativeErrorCode}}
  [ordered]@{name=$name;executable=$file;sha256=(Hash $file);arguments=$arguments;exitCode=$exit;launchError=$launchError;nativeErrorCode=$code;timedOut=$timeout;passed=($null -eq $launchError -and -not $timeout -and $exit -eq 0)}
}
$smoke=Invoke-Case 'host-smoke' 'install/bin/Producer.exe' @('--smoke',('"'+(Join-Path $run 'host')+'"'));$cases+=$smoke
# Do not retry a policy refusal using another executable or load mechanism.
if($smoke.launchError){$blocked=$true}
if(-not $blocked) {
  $arguments=@(('"'+(Join-Path $run 'core')+'"'))
  $reference=$null
  if($ReferenceSegment){$reference=[IO.Path]::GetFullPath($ReferenceSegment);$arguments+=('"'+$reference+'"')}
  if($ReferenceStyleDirectory){$arguments+=('"'+[IO.Path]::GetFullPath($ReferenceStyleDirectory)+'"')}
  if($ReferenceCollection){$arguments+=('"'+[IO.Path]::GetFullPath($ReferenceCollection)+'"')}
  $core=Invoke-Case 'core' 'build/Release/producer_core_tests.exe' $arguments;$cases+=$core
  if($core.launchError){$blocked=$true}
}
if($Playback -and -not $blocked){$cases+=Invoke-Case 'playback-api' 'install/bin/Producer.exe' @('--playback-smoke',('"'+(Join-Path $run 'playback')+'"'));if($cases[-1].launchError){$blocked=$true}}
if($GroupPlaybackSegment -and -not $blocked){$cases+=Invoke-Case 'group-playback-api' 'install/bin/Producer.exe' @('--group-playback-smoke',('"'+(Join-Path $run 'group-playback')+'"'),('"'+[IO.Path]::GetFullPath($GroupPlaybackSegment)+'"'));if($cases[-1].launchError){$blocked=$true}}
if($DlsPlayback -and -not $blocked -and (-not ($DlsPlaybackCoreWave -or $DlsPlaybackCorePcm -or $DlsPlaybackCoreResize -or $DlsPlaybackCoreRegion -or $DlsPlaybackCoreSaveAs -or $DlsPlaybackCoreDelete) -or $core.passed)){$cases+=Invoke-Case 'dls-playback-api' 'install/bin/Producer.exe' @('--dls-playback-smoke',('"'+(Join-Path $run 'dls-playback')+'"'),('"'+[IO.Path]::GetFullPath($DlsPlaybackCollection)+'"'),([string]$DlsPlaybackInstrument));if($cases[-1].launchError){$blocked=$true}}
if($StylePlayback -and -not $blocked){$cases+=Invoke-Case 'style-playback-api' 'install/bin/Producer.exe' @('--style-playback-smoke',('"'+(Join-Path $run 'style-playback')+'"'),('"'+[IO.Path]::GetFullPath($ReferenceSegment)+'"'),('"'+[IO.Path]::GetFullPath($ReferenceStyleDirectory)+'"'));if($cases[-1].launchError){$blocked=$true}}
$referenceRecord=$null;if($ReferenceSegment){$referenceRecord=[ordered]@{path=[IO.Path]::GetFullPath($ReferenceSegment);sha256=(Hash $ReferenceSegment)}}
$groupRecord=$null;if($GroupPlaybackSegment){$groupRecord=[ordered]@{path=[IO.Path]::GetFullPath($GroupPlaybackSegment);sha256=(Hash $GroupPlaybackSegment)}}
$styleRecord=$null;if($ReferenceStyleDirectory){$styleRecord=[ordered]@{directory=[IO.Path]::GetFullPath($ReferenceStyleDirectory);files=@(Get-ChildItem -LiteralPath $ReferenceStyleDirectory -File | Where-Object Extension -in '.stp','.sty' | ForEach-Object {[ordered]@{path=$_.FullName;sha256=(Hash $_.FullName)}});ownedCopy=$(if(Test-Path -LiteralPath (Join-Path $run 'core/reference-style.stp')){[ordered]@{path=(Join-Path $run 'core/reference-style.stp');sha256=(Hash (Join-Path $run 'core/reference-style.stp'))}})}}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);osVersion=[Environment]::OSVersion.VersionString;user=[Environment]::UserName;
  cases=$cases;groupPlaybackInput=$groupRecord;referenceInput=$referenceRecord;referenceStyles=$styleRecord;referenceCollection=$(if($ReferenceCollection){[ordered]@{path=[IO.Path]::GetFullPath($ReferenceCollection);sha256=(Hash $ReferenceCollection)}});executionBlocked=$blocked;coreExecuted=(@($cases|Where-Object {$_.name -eq 'core' -and $null -eq $_.launchError}).Count -eq 1);dlsPlaybackInstrument=$DlsPlaybackInstrument;dlsPlaybackCoreWave=[bool]$DlsPlaybackCoreWave;dlsPlaybackCorePcm=[bool]$DlsPlaybackCorePcm;dlsPlaybackCoreResize=[bool]$DlsPlaybackCoreResize;dlsPlaybackCoreRegion=[bool]$DlsPlaybackCoreRegion;dlsPlaybackCoreSaveAs=[bool]$DlsPlaybackCoreSaveAs;dlsPlaybackCoreDelete=[bool]$DlsPlaybackCoreDelete;dlsPlaybackCollection=$(if($DlsPlayback -and (Test-Path -LiteralPath $DlsPlaybackCollection -PathType Leaf)){[ordered]@{path=[IO.Path]::GetFullPath($DlsPlaybackCollection);sha256=(Hash $DlsPlaybackCollection)}});uiAcceptance='unexecuted';separateProcessReload='unexecuted';audioAcceptance='unexecuted';fullAcceptancePassed=$false
}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if($blocked -or @($cases|Where-Object {-not $_.passed}).Count){throw 'Product runtime checks incomplete or failed; inspect evidence'}

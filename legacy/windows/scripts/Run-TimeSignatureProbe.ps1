[CmdletBinding()]
param([string]$BuildDirectory='work/build/time-signature',[string]$ModulePath='work/producer/app/TimeSigStripMgr.dll',[switch]$Candidate,[string]$BuildSummaryPath,[switch]$Connection,[switch]$Style)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$build=if([IO.Path]::IsPathRooted($BuildDirectory)){$BuildDirectory}else{Join-Path $repo $BuildDirectory}
$dll=if([IO.Path]::IsPathRooted($ModulePath)){$ModulePath}else{Join-Path $repo $ModulePath}
$dllHash=(Get-FileHash -LiteralPath $dll).Hash.ToLowerInvariant()
if(-not $Candidate -and $dllHash -ne '898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7'){throw 'Original TimeSig identity mismatch'}
if($Connection -and $Style){throw 'Select one probe profile'}
$probeName=if($Style){'time_signature_style_probe.exe'}elseif($Connection){'time_signature_connection_probe.exe'}else{'time_signature_probe.exe'}
$probe=Join-Path $build ('Release/'+$probeName)
$buildSummarySha256=$null
if($BuildSummaryPath){
  $buildRecordPath=[IO.Path]::GetFullPath($BuildSummaryPath)
  $buildRecord=Get-Content -LiteralPath $buildRecordPath -Raw|ConvertFrom-Json
  if(-not $buildRecord.passed -or -not $buildRecord.sourceSnapshotUnchanged -or $buildRecord.buildDirectory -ine [IO.Path]::GetFullPath($build)){throw 'Expected verified source snapshot build'}
  $probeOutput=@($buildRecord.outputs|Where-Object path -eq ('build/Release/'+$probeName))
  if($probeOutput.Count -ne 1 -or (Get-FileHash -LiteralPath $probe).Hash.ToLowerInvariant() -ne $probeOutput[0].sha256){throw 'Built probe identity mismatch'}
  foreach($source in $buildRecord.sources){
    if((Get-FileHash -LiteralPath (Join-Path $buildRecord.sourceRoot $source.path)).Hash.ToLowerInvariant() -ne $source.sha256 -or
       (Get-FileHash -LiteralPath (Join-Path $repo $source.path)).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Built/current source mismatch'}
  }
  if($Candidate){$dllOutput=@($buildRecord.outputs|Where-Object path -eq 'build/Release/TimeSigStripMgr.dll');if($dllOutput.Count -ne 1 -or $dllHash -ne $dllOutput[0].sha256){throw 'Built candidate identity mismatch'}}
  $buildSummarySha256=(Get-FileHash -LiteralPath $buildRecordPath).Hash.ToLowerInvariant()
}
$kind=if($Candidate){'candidate'}else{'reference'}
$category=if($Style){'time-signature-style'}elseif($Connection){'time-signature-connection'}else{'time-signature'}
$run=Join-Path $repo ('work/'+$kind+'/'+$category+'/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run | Out-Null
$sourcePaths=@('tests/native/time_signature_probe.cpp','tests/native/time_signature/CMakeLists.txt','src/compat/producer_ids.h','src/compat/strip_manager.h','src/compat/time_signature.h','scripts/Run-TimeSignatureProbe.ps1','scripts/Inspect-TimeSignatureAbi.mjs',
  'src/time_signature/time_signature_map.h','src/time_signature/time_signature_map.cpp','src/time_signature/time_signature_dll.cpp','src/time_signature/time_signature_dll.def','src/time_signature/CMakeLists.txt')
$sourcePaths+=@('tests/native/time_signature_connection_probe.cpp','src/compat/strip.h','src/compat/prop_page_object.h','src/compat/timeline_services.h')
$sourcePaths+=@('tests/native/time_signature_style_probe.cpp','tests/native/reference_timeline.h')
$sourceHashes=@($sourcePaths|ForEach-Object{[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash.ToLowerInvariant()}})
foreach($source in $sourceHashes){
  $destination=Join-Path $run ('sources/'+$source.path)
  New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
  Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $destination
  if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Source snapshot changed'}
}
$exitCode=$null;$launchError=$null;$timedOut=$false
$arguments=@(('"{0}"' -f $dll),('"{0}"' -f $run))
$timelineMetadata=$null
if($Style){
  $timelineFile=Join-Path $repo 'work/producer/app/Timeline.dll'
  $timelineHash=(Get-FileHash -LiteralPath $timelineFile).Hash.ToLowerInvariant()
  if($timelineHash -ne 'bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176'){throw 'Original Timeline identity mismatch'}
  $timelineMetadata=[ordered]@{path=$timelineFile;sha256=$timelineHash}
  $arguments+=('"{0}"' -f $timelineFile)
}
try{
  $probeProcess=Start-Process -FilePath $probe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'probe.jsonl') -RedirectStandardError (Join-Path $run 'stderr.txt')
  $timedOut=-not $probeProcess.WaitForExit(15000)
  if($timedOut){Stop-Process -Id $probeProcess.Id;$probeProcess.WaitForExit()}
  $probeProcess.Refresh();$exitCode=$probeProcess.ExitCode
}catch{$launchError=$_.Exception.Message}
$scope=if($Style){'TimeSig real original Timeline with synthetic Style manager and signatures. No window, runtime synchronization, UI, playback or registration.'}elseif($Connection){'TimeSig Timeline fixture connection, strip properties, notification registration/unregistration and ownership. No real Timeline, UI, editing, playback or registration.'}else{'TimeSig creation, standalone parameters/persistence and owned service QI/refcount diagnostics; fake services disconnected during loading; no Timeline connection, UI or playback'}
[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');implementation=$kind;connectionProbe=[bool]$Connection;styleProbe=[bool]$Style;timeline=$timelineMetadata;dll=$dll;dllSha256=$dllHash;probeSha256=(Get-FileHash -LiteralPath $probe).Hash.ToLowerInvariant();buildSummary=$BuildSummaryPath;buildSummarySha256=$buildSummarySha256;exitCode=$exitCode;launchError=$launchError;timedOut=$timedOut;sources=$sourceHashes;sourceSnapshot='sources';registryRegistrationInvoked=$false;scope=$scope} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if($launchError -or $timedOut -or $exitCode -ne 0){throw 'TimeSig probe failed; inspect retained evidence'}

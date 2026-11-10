[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$RecorderBuildSummaryPath,[Parameter(Mandatory)][string]$Recorder,[Parameter(Mandatory)][string]$Segment,[string]$Node='node',[ValidateSet('notes','lifecycle','motif-lifecycle','tempo','crud','variation-first','variation-last','motif-repeat','motif-standalone')][string]$Profile='notes',[switch]$SilenceControl,[string]$MotifName='',[switch]$DlsAudio,[ValidateRange(0,3072)][int]$MotifDelayClocks=0,[ValidateRange(0,4)][int]$MotifBoundary=0,[switch]$QuickResponse,[switch]$Secondary,[switch]$MotifTempoAudio)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if($MotifTempoAudio -and ($Profile -ne 'motif-standalone' -or $SilenceControl -or $DlsAudio)){throw 'Tempo audio requires finite standalone Motif profile'}
if($DlsAudio -and ($Profile -ne 'motif-standalone' -or $SilenceControl)){throw 'DLS audio profile requires standalone Motif playback'}
if(($MotifDelayClocks -or $MotifBoundary -or $QuickResponse -or $Secondary) -and $Profile -ne 'motif-standalone'){throw 'Playback options require standalone Motif profile'}
if($Profile.StartsWith('motif-') -and (-not $MotifName -or $MotifName.Contains('"') -or $MotifName.Contains([char]0))){throw 'Explicit supported Motif name required'}
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256){throw 'Snapshot changed'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe'
$identity=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Producer identity mismatch'}
$recorderPath=[IO.Path]::GetFullPath($Recorder);$inputPath=[IO.Path]::GetFullPath($Segment)
$recorderSummaryPath=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$recorderSummary=Get-Content -LiteralPath $recorderSummaryPath -Raw|ConvertFrom-Json
if(-not $recorderSummary.passed -or (Hash $recorderPath) -ne $recorderSummary.sha256){throw 'Recorder build identity mismatch'}
foreach($s in $recorderSummary.sources){if((Hash (Join-Path (Split-Path $recorderSummaryPath -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder snapshot changed'}}
$run=Join-Path $repo ('work/acceptance/audio-loopback/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$files=@($inputPath,(Join-Path (Split-Path $inputPath -Parent) 'Heartlnd.stp'),(Join-Path (Split-Path $inputPath -Parent) 'owned.dls'))|Where-Object {Test-Path -LiteralPath $_}
$inputs=@($files|ForEach-Object {[ordered]@{path=$_;sha256=Hash $_}})
$capture=Start-Process -FilePath $recorderPath -ArgumentList @(('"'+$run+'"'),'16') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'capture.stdout.txt') -RedirectStandardError (Join-Path $run 'capture.stderr.txt')
$ready=Join-Path $run 'ready.json';$limit=[DateTime]::UtcNow.AddSeconds(5)
while(-not (Test-Path -LiteralPath $ready) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
if(-not (Test-Path -LiteralPath $ready)){throw ('Recorder not ready; evidence '+$run)}
$readyUtc=[DateTime]::UtcNow.ToString('o');Start-Sleep -Seconds 2
$startUtc=$null;$endUtc=$null;$exit=$null;$pidValue=$null
$command=if($Profile -eq 'lifecycle'){'--audio-lifecycle'}else{'--note-observe'}
if($Profile -eq 'motif-repeat'){$command='--motif-observe'}
if($Profile -eq 'motif-standalone'){$command='--style-motif-scheduled-observe'}
if($Profile -eq 'motif-lifecycle'){$command='--motif-lifecycle'}
$caseName=if($Profile -in @('lifecycle','motif-lifecycle')){'audio-lifecycle'}elseif($Profile -eq 'tempo'){'audio-tempo'}elseif($Profile -eq 'crud' -or $Profile.StartsWith('variation-') -or $Profile -in @('motif-repeat','motif-standalone')){'audio-pattern'}else{'notes-api'}
if(-not $SilenceControl){
  $startUtc=[DateTime]::UtcNow.ToString('o')
  $observationDirectory=if($Profile.StartsWith('variation-') -or $Profile -in @('motif-repeat','motif-standalone')){'crud'}else{$Profile}
  if($Profile -eq 'motif-lifecycle'){$observationDirectory='lifecycle'}
  $playerArguments=@($command,('"'+(Join-Path $run $observationDirectory)+'"'),('"'+$inputPath+'"'));if($Profile.StartsWith('motif-')){$playerArguments+=('"'+$MotifName+'"')}
  if($Profile -eq 'motif-standalone'){$playerArguments+=@([string]$MotifDelayClocks,[string]$MotifBoundary,([string][int](-not $QuickResponse)),([string][int]([bool]$Secondary)))}
  $player=Start-Process -FilePath $exe -ArgumentList $playerArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'player.stdout.txt') -RedirectStandardError (Join-Path $run 'player.stderr.txt')
  $pidValue=$player.Id
  if(-not $player.WaitForExit(14000)){throw ('Player timeout; evidence '+$run)}
  $player.Refresh();$exit=$player.ExitCode;$endUtc=[DateTime]::UtcNow.ToString('o')
}
if(-not $capture.WaitForExit(20000)){throw ('Recorder timeout; evidence '+$run)};$capture.Refresh()
[ordered]@{schema=1;profile=$Profile;readyUtc=$readyUtc;playerStartUtc=$startUtc;playerEndUtc=$endUtc;playerPid=$pidValue;playerExitCode=$exit;captureExitCode=$capture.ExitCode;silenceControl=[bool]$SilenceControl;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;producer=$exe;producerSha256=Hash $exe;recorder=$recorderPath;recorderSha256=Hash $recorderPath;recorderBuildSummary=$recorderSummaryPath;recorderBuildSummarySha256=Hash $recorderSummaryPath;driverSha256=Hash $PSCommandPath;inputs=$inputs;cases=@([ordered]@{name=$caseName;executable=$exe;sha256=Hash $exe;passed=(-not $SilenceControl -and $exit -eq 0)});scope='Endpoint digital recording; acoustic speaker and GUI not tested';fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output $run
if($capture.ExitCode -ne 0 -or (-not $SilenceControl -and $exit -ne 0)){throw 'Capture/player failed'}
$auditor=if($Profile -in @('lifecycle','motif-lifecycle') -and -not $SilenceControl){'Inspect-AudioLifecycle.mjs'}elseif($Profile -eq 'tempo' -and -not $SilenceControl){'Inspect-TempoAudio.mjs'}else{'Inspect-LoopbackAudio.mjs'}
if($DlsAudio){$auditor='Inspect-MotifDlsAudio.mjs'}
if($MotifTempoAudio){if($Profile -ne 'motif-standalone' -or $SilenceControl -or $DlsAudio){throw 'Tempo audio requires finite standalone Motif profile'};$auditor='Inspect-MotifTempoAudio.mjs'}
& $Node (Join-Path $PSScriptRoot $auditor) $run | Set-Content -LiteralPath (Join-Path $run 'analysis.stdout.txt') -Encoding UTF8
if($LASTEXITCODE -ne 0){throw ('Audio verification failed; '+$run)}
if(-not $SilenceControl){& (Join-Path $PSScriptRoot 'Inspect-ProductModules.ps1') -RunPath (Join-Path $run 'run.json') -CaseName $caseName}



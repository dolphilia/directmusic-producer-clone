[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildSummaryPath,
    [Parameter(Mandatory)][string]$ObserverPath,
    [Parameter(Mandatory)][string]$ScriptPath,
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [string]$RecorderBuildSummaryPath='work/build/audio-capture/20261006T211007927Z/build-summary.json'
)
$ErrorActionPreference='Stop'
function Hash([string]$path){(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()}
$buildPath=[IO.Path]::GetFullPath($BuildSummaryPath)
$build=Get-Content -LiteralPath $buildPath -Raw|ConvertFrom-Json
if(-not $build.passed){throw 'Successful saved candidate required'}
foreach($source in $build.sources){if((Hash (Join-Path $build.sourceRoot $source.path)) -ne $source.sha256){throw 'Saved product source changed'}}
$core=@($build.outputs|Where-Object path -eq 'build/Release/producer_core.lib')[0]
$corePath=Join-Path (Split-Path $buildPath -Parent) $core.path
if((Hash $corePath) -ne $core.sha256){throw 'Saved core changed'}
$observer=[IO.Path]::GetFullPath($ObserverPath)
$observerUnit=Split-Path (Split-Path (Split-Path $observer -Parent) -Parent) -Parent
$provenancePath=Join-Path $observerUnit 'provenance.json'
$provenance=Get-Content -LiteralPath $provenancePath -Raw|ConvertFrom-Json
$observerHash=Hash $observer
$repo=Split-Path $PSScriptRoot -Parent
if($provenance.candidate -ne (Split-Path (Split-Path $buildPath -Parent) -Leaf) -or $observerHash -ne $provenance.executable.sha256 -or [IO.Path]::GetFullPath((Join-Path $repo $provenance.executable.path)) -ne $observer){throw 'Observer candidate or executable provenance mismatch'}
foreach($source in $provenance.sources){if((Hash (Join-Path $repo $source.path)) -ne $source.sha256){throw 'Observer source changed'}}
foreach($artifact in @($provenance.build,$provenance.savedConductor,$provenance.savedCore)){if((Hash (Join-Path $repo $artifact.path)) -ne $artifact.sha256){throw 'Observer build provenance changed'}}
if([IO.Path]::GetFullPath((Join-Path $repo $provenance.savedCore.path)) -ne [IO.Path]::GetFullPath($corePath) -or $provenance.savedCore.sha256 -ne $core.sha256){throw 'Observer linked core differs from candidate'}
$conductor=@($build.sources|Where-Object path -eq 'src/producer/conductor.cpp')[0]
if(-not $conductor -or [IO.Path]::GetFullPath((Join-Path $repo $provenance.savedConductor.path)) -ne [IO.Path]::GetFullPath((Join-Path $build.sourceRoot $conductor.path)) -or $provenance.savedConductor.sha256 -ne $conductor.sha256){throw 'Observer compiled Conductor differs from candidate'}
$recorderPath=[IO.Path]::GetFullPath($RecorderBuildSummaryPath)
$recorder=Get-Content -LiteralPath $recorderPath -Raw|ConvertFrom-Json
if(-not $recorder.passed -or (Hash $recorder.executable) -ne $recorder.sha256){throw 'Recorder identity mismatch'}
foreach($source in $recorder.sources){if((Hash (Join-Path (Split-Path $recorderPath -Parent) ('sources/'+$source.path))) -ne $source.sha256){throw 'Saved recorder source changed'}}
$inputPath=[IO.Path]::GetFullPath($ScriptPath)
$inputs=@(Get-ChildItem -LiteralPath (Split-Path $inputPath -Parent) -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
if(Test-Path -LiteralPath $evidence){throw 'Fresh evidence directory required'}
New-Item -ItemType Directory -Path $evidence|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $evidence 'driver.ps1')
$record=[ordered]@{schema=1;candidate=Split-Path (Split-Path $buildPath -Parent) -Leaf;buildSummary=$buildPath;buildSummarySha256=Hash $buildPath;savedCore=$corePath;savedCoreSha256=$core.sha256;observer=$observer;observerSha256=$observerHash;observerProvenance=$provenancePath;observerProvenanceSha256=Hash $provenancePath;recorder=$recorder.executable;recorderSha256=$recorder.sha256;recorderBuildSummary=$recorderPath;recorderBuildSummarySha256=Hash $recorderPath;inputs=$inputs;durationSeconds=32;state='starting';driverSha256=Hash $PSCommandPath;forcedTermination=$false;scope='Native Farm NoteObserver and synchronized WASAPI; main GUI is separate';fullAcceptance=$false}
function SaveRecord{$record|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $evidence 'run.json') -Encoding utf8}
SaveRecord
$captureArguments=@(('"'+$evidence+'"'),'32','--silent-keepalive')
$capture=Start-Process -FilePath $recorder.executable -ArgumentList $captureArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $evidence 'capture.stdout.txt') -RedirectStandardError (Join-Path $evidence 'capture.stderr.txt')
$record['captureProcessId']=$capture.Id
$record['captureStartedUtc']=$capture.StartTime.ToUniversalTime().ToString('o')
$record['captureArguments']=$captureArguments
SaveRecord
$limit=[DateTime]::UtcNow.AddSeconds(5)
while(-not (Test-Path -LiteralPath (Join-Path $evidence 'ready.json')) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 25}
if(-not (Test-Path -LiteralPath (Join-Path $evidence 'ready.json'))){throw 'Recorder not ready; preserve process and evidence'}
$record['readyUtc']=[DateTime]::UtcNow.ToString('o')
$observerArguments=@(('"'+$inputPath+'"'),('"'+(Join-Path $evidence 'observations')+'"'))
$player=Start-Process -FilePath $observer -ArgumentList $observerArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $evidence 'observer.stdout.txt') -RedirectStandardError (Join-Path $evidence 'observer.stderr.txt')
$record['observerArguments']=$observerArguments
$record['observerProcessId']=$player.Id
$record['observerStartedUtc']=$player.StartTime.ToUniversalTime().ToString('o')
$record.state='recording';SaveRecord
Write-Output ('Evidence: '+$evidence)
$observerExited=$player.WaitForExit(45000)
$record['observerNormalExit']=$observerExited
if($observerExited){$player.Refresh();$record['observerExitCode']=$player.ExitCode;$record['observerExitUtc']=$player.ExitTime.ToUniversalTime().ToString('o')}
$captureExited=$capture.WaitForExit(15000)
$record['captureNormalExit']=$captureExited
if($captureExited){$capture.Refresh();$record['captureExitCode']=$capture.ExitCode;$record['captureExitUtc']=$capture.ExitTime.ToUniversalTime().ToString('o')}
$record['inputsChanged']=@($inputs|Where-Object {(Hash $_.path) -ne $_.sha256})
$record.state=if($observerExited -and $captureExited){'exited'}else{'still-live'}
SaveRecord
if(-not $observerExited -or -not $captureExited){throw 'Process still live; do not terminate or blindly rerun'}
if($player.ExitCode -ne 0 -or $capture.ExitCode -ne 0 -or $record.inputsChanged.Count){throw 'Native/audio execution failed or input changed; preserve evidence'}
Get-Content -LiteralPath (Join-Path $evidence 'observer.stdout.txt')

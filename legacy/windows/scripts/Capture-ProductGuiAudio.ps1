[CmdletBinding()]
param([Parameter(Mandatory)][string]$GuiRun,[string]$RecorderBuildSummaryPath='work/build/audio-capture/20261006T211007927Z/build-summary.json',[ValidateRange(16,300)][int]$DurationSeconds=32,[string[]]$AdditionalInputs=@(),[switch]$SilentKeepAlive)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$gui=[IO.Path]::GetFullPath($GuiRun);$launch=Get-Content -LiteralPath (Join-Path $gui 'launch.json') -Raw|ConvertFrom-Json
$build=Get-Content -LiteralPath $launch.buildSummary -Raw|ConvertFrom-Json
if(-not $build.passed -or (Hash $launch.buildSummary) -ne $launch.buildSummarySha256 -or (Hash $launch.executable) -ne $launch.exeSha256){throw 'GUI/build identity mismatch'}
foreach($s in $build.sources){if((Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256){throw 'Saved source mismatch'}}
$player=Get-Process -Id $launch.processId
if($player.Path -ne $launch.executable){throw 'GUI process identity mismatch'}
$recorderSummary=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$recorder=Get-Content -LiteralPath $recorderSummary -Raw|ConvertFrom-Json
if(-not $recorder.passed -or (Hash $recorder.executable) -ne $recorder.sha256){throw 'Recorder identity mismatch'}
foreach($s in $recorder.sources){if((Hash (Join-Path (Split-Path $recorderSummary -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$dir=Join-Path $gui ('audio-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');Copy-Item -LiteralPath (Join-Path $gui 'launch.json') -Destination (Join-Path $dir 'gui-launch-at-capture.json')
$inputs=@($AdditionalInputs|ForEach-Object {$p=[IO.Path]::GetFullPath($_);[ordered]@{path=$p;sha256=Hash $p}})
$record=[ordered]@{schema=1;guiRun=$gui;processId=$launch.processId;executable=$launch.executable;exeSha256=$launch.exeSha256;buildSummary=$launch.buildSummary;buildSummarySha256=$launch.buildSummarySha256;recorder=$recorder.executable;recorderSha256=$recorder.sha256;recorderBuildSummary=$recorderSummary;recorderBuildSummarySha256=Hash $recorderSummary;inputs=$inputs;driverSha256=Hash $PSCommandPath;state='recording';readyUtc=$null;captureExitCode=$null;scope='Capture current GUI Play only; GUI actions and PCM acceptance separate';fullAcceptance=$false}
function SaveRecord{$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8}
$record['durationSeconds']=$DurationSeconds
$record['silentKeepAlive']=[bool]$SilentKeepAlive
$captureArguments=@(('"'+$dir+'"'),[string]$DurationSeconds)
if($SilentKeepAlive){$captureArguments+='--silent-keepalive'}
$record['recorderArguments']=$captureArguments
$capture=Start-Process -FilePath $recorder.executable -ArgumentList $captureArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'capture.stdout.txt') -RedirectStandardError (Join-Path $dir 'capture.stderr.txt')
SaveRecord;Write-Output ('Evidence: '+$dir)
$limit=[DateTime]::UtcNow.AddSeconds(5)
while(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json')) -and [DateTime]::UtcNow -lt $limit -and -not $capture.HasExited){Start-Sleep -Milliseconds 50}
if(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json'))){
    $capture.Refresh();$record.state=if($capture.HasExited){'exited-before-ready'}else{'not-ready'}
    if($capture.HasExited){$record.captureExitCode=$capture.ExitCode};SaveRecord
    throw 'Recorder not ready; preserve evidence'
}
$record.readyUtc=[DateTime]::UtcNow.ToString('o')
$captureReady=Get-Content -LiteralPath (Join-Path $dir 'ready.json') -Raw|ConvertFrom-Json
$record['captureReadySha256']=Hash (Join-Path $dir 'ready.json')
$record['timingBasis']=if($captureReady.schema -eq 2){'recorder-start-utc-qpc'}else{'legacy-ready-poll; calibrated GUI timing unavailable'}
if($captureReady.schema -eq 2){
    $captureUtcMid=[long][decimal]::Truncate([decimal]$captureReady.utcBeforeFileTime+([decimal]$captureReady.utcAfterFileTime-[decimal]$captureReady.utcBeforeFileTime)/2)
    $record['captureStartUtc']=[DateTime]::FromFileTimeUtc([long]$captureUtcMid).ToString('o')
}
SaveRecord
if(-not $capture.WaitForExit(($DurationSeconds+15)*1000)){throw ('Recorder still live PID '+$capture.Id+'; inspect before replay')}
$capture.Refresh();$record.captureExitCode=$capture.ExitCode;$record.state='exited';SaveRecord
if($capture.ExitCode -ne 0){throw 'Recorder failed; preserve evidence'}

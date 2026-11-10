[CmdletBinding()]
param([Parameter(Mandatory)][string]$RecorderBuildSummaryPath,[string]$Node='C:/Users/dolph/Tools/node-v24.18.1-win-x64/node.exe')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($RecorderBuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or (Hash $summary.executable) -ne $summary.sha256){throw 'Verified recorder build required'}
foreach($s in $summary.sources){if((Hash (Join-Path (Split-Path $summaryPath -Parent) ('sources/'+$s.path))) -ne $s.sha256){throw 'Recorder source mismatch'}}
$repo=Split-Path $PSScriptRoot -Parent
$dir=Join-Path $repo ('work/acceptance/audio-capture-clock/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$capture=Start-Process -FilePath $summary.executable -ArgumentList @(('"'+$dir+'"'),'6') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'capture.stdout.txt') -RedirectStandardError (Join-Path $dir 'capture.stderr.txt')
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;recorder=$summary.executable;recorderSha256=$summary.sha256;processId=$capture.Id;readyObservedUtc=$null;captureExitCode=$null;driverSha256=Hash $PSCommandPath;fullAcceptancePassed=$false}
function SaveRecord{$record|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8}
SaveRecord;Write-Output ('Evidence: '+$dir)
$deadline=[DateTime]::UtcNow.AddSeconds(5)
while(-not (Test-Path -LiteralPath (Join-Path $dir 'ready.json')) -and [DateTime]::UtcNow -lt $deadline -and -not $capture.HasExited){Start-Sleep -Milliseconds 20}
if(Test-Path -LiteralPath (Join-Path $dir 'ready.json')){$record.readyObservedUtc=[DateTime]::UtcNow.ToString('o');SaveRecord}
if(-not $capture.WaitForExit(10000)){throw ('Recorder still live PID '+$capture.Id+'; no relaunch or forced termination')}
$capture.Refresh();$record.captureExitCode=$capture.ExitCode;SaveRecord
if($capture.ExitCode -ne 0 -or -not $record.readyObservedUtc){throw 'Recorder capture or ready failed'}
& $Node (Join-Path $PSScriptRoot 'Inspect-AudioCaptureClock.mjs') $dir
if($LASTEXITCODE -ne 0){throw 'Recorder clock verification failed'}

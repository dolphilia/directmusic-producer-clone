[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Project,[string[]]$InputPaths=@())
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified successful saved build required'}
foreach($source in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $source.path)) -ne $source.sha256){throw 'Saved source changed'}}
$output=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($output.Count -ne 1){throw 'Installed executable missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path;if((Hash $exe) -ne $output[0].sha256){throw 'Executable changed'}
$projectPath=[IO.Path]::GetFullPath($Project)
$inputs=@(@($projectPath)+@($InputPaths|ForEach-Object {[IO.Path]::GetFullPath($_)})|ForEach-Object {[ordered]@{path=$_;sha256=(Hash $_)}})
$run=Join-Path $repo ('work/acceptance/product-project-gui/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'launcher.ps1')
$arguments=@('--open-project',('"'+$projectPath+'"'))
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);driverSha256=(Hash $PSCommandPath);executable=$exe;exeSha256=$output[0].sha256;arguments=$arguments;inputs=$inputs;processId=$null;state='launching';exitCode=$null;launchError=$null;timedOut=$false;passed=$false;uiAcceptance='separate observation required';audioAcceptance='unverified';fullAcceptancePassed=$false}
function SaveRecord {$record|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'launch.json') -Encoding utf8}
SaveRecord
try {
    # The user explicitly requested reopening the interactive verification app.
    $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Normal -PassThru
    $record.processId=$process.Id;$record.state='running';SaveRecord
    Write-Output ('Evidence: '+$run)
    $record.timedOut=-not $process.WaitForExit(900000)
    if($record.timedOut){$record.state='still running; no forced termination'}else{$process.Refresh();$record.exitCode=$process.ExitCode;$record.state='exited';$record.passed=$process.ExitCode -eq 0}
}catch{$record.launchError=$_.Exception.Message;$record.state='launch failed'}
SaveRecord
if($record.launchError){throw $record.launchError}

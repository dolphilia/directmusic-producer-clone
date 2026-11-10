[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Project,[string[]]$InputPaths=@())
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath)
$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified successful saved build required'}
foreach($source in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $source.path)) -ne $source.sha256){throw 'Saved source changed'}}
$output=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($output.Count -ne 1){throw 'Installed executable missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path
if((Hash $exe) -ne $output[0].sha256){throw 'Executable changed'}
$projectPath=[IO.Path]::GetFullPath($Project)
$inputs=@(@($projectPath)+@($InputPaths|ForEach-Object {[IO.Path]::GetFullPath($_)})|ForEach-Object {[ordered]@{path=$_;sha256=(Hash $_)}})
$run=Join-Path $repo ('work/acceptance/product-project-gui-preparation/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'preparation.ps1')
# Launch on the interactive Computer Use desktop separately. Preparation is not
# evidence of launch, GUI acceptance, normal exit, or audible output.
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);driverSha256=(Hash $PSCommandPath);executable=$exe;exeSha256=$output[0].sha256;project=$projectPath;inputs=$inputs;sourceCount=@($summary.sources).Count;preparationPassed=$true;launchVerified=$false;uiAcceptance='not executed';audioAcceptance='not executed';fullAcceptancePassed=$false}
$record|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'preparation.json') -Encoding utf8
$record|ConvertTo-Json -Depth 7
Write-Output ('Evidence: '+$run)

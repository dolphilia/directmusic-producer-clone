[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$farmRuntimeOutput=@(& (Join-Path $PSScriptRoot 'Test-RegressionManifest.ps1') -BuildSummaryPath $BuildSummaryPath -Only 'farm-script-runtime')
$farmRuntimeOutput | Write-Output
$farmRuntimeEvidence=@($farmRuntimeOutput | Where-Object {$_ -like 'Evidence: *'})
if($farmRuntimeEvidence.Count -ne 1){throw 'Exactly one Farm runtime regression evidence path required'}
$farmRuntimeRun=Get-Content -LiteralPath (Join-Path $farmRuntimeEvidence[0].Substring(10) 'run.json') -Raw | ConvertFrom-Json
if(@($farmRuntimeRun.results).Count -ne 1 -or $farmRuntimeRun.results[0].id -ne 'farm-script-runtime' -or $farmRuntimeRun.results[0].status -ne '合格' -or $farmRuntimeRun.results[0].exitCode -ne 0){throw 'Farm Script runtime regression did not pass; preserve producing run'}

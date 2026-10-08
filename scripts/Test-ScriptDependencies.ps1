[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$farmOwnedOutput=@(& (Join-Path $PSScriptRoot 'Test-RegressionManifest.ps1') -BuildSummaryPath $BuildSummaryPath -Only 'script-dependencies')
$farmOwnedOutput | Write-Output
$farmOwnedEvidence=@($farmOwnedOutput | Where-Object {$_ -like 'Evidence: *'})
if($farmOwnedEvidence.Count -ne 1){throw 'Exactly one owned Script regression evidence path required'}
$farmOwnedRun=Get-Content -LiteralPath (Join-Path $farmOwnedEvidence[0].Substring(10) 'run.json') -Raw | ConvertFrom-Json
if(@($farmOwnedRun.results).Count -ne 1 -or $farmOwnedRun.results[0].id -ne 'script-dependencies' -or $farmOwnedRun.results[0].status -ne '合格' -or $farmOwnedRun.results[0].exitCode -ne 0){throw 'Owned Script dependency regression did not pass; preserve producing run'}

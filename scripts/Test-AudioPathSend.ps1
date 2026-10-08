[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$sendTestOutput=@(& (Join-Path $PSScriptRoot 'Test-RegressionManifest.ps1') -BuildSummaryPath $BuildSummaryPath -Only 'audio-path-send')
$sendTestOutput | Write-Output
$sendTestEvidence=@($sendTestOutput | Where-Object {$_ -like 'Evidence: *'})
if($sendTestEvidence.Count -ne 1){throw 'Exactly one Send document regression evidence path required'}
$sendTestRun=Get-Content -LiteralPath (Join-Path $sendTestEvidence[0].Substring(10) 'run.json') -Raw | ConvertFrom-Json
if(@($sendTestRun.results).Count -ne 1 -or $sendTestRun.results[0].id -ne 'audio-path-send' -or $sendTestRun.results[0].status -ne '合格' -or $sendTestRun.results[0].exitCode -ne 0){throw 'Send document regression did not pass; preserve producing run'}

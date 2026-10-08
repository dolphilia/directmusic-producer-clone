[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
# Registered native modes: '--waves-reverb-document', '--waves-reverb-runtime'.
foreach($wavesMode in @('waves-reverb-document','waves-reverb-runtime')){
    $wavesOutput=@(& (Join-Path $PSScriptRoot 'Test-RegressionManifest.ps1') -BuildSummaryPath $BuildSummaryPath -Only $wavesMode)
    $wavesOutput | Write-Output
    $wavesEvidence=@($wavesOutput | Where-Object {$_ -like 'Evidence: *'})
    if($wavesEvidence.Count -ne 1){throw 'Exactly one Waves regression evidence path required'}
    $wavesResult=Get-Content -LiteralPath (Join-Path $wavesEvidence[0].Substring(10) 'run.json') -Raw | ConvertFrom-Json
    if(@($wavesResult.results).Count -ne 1 -or $wavesResult.results[0].id -ne $wavesMode -or $wavesResult.results[0].status -ne '合格' -or $wavesResult.results[0].exitCode -ne 0){throw 'Waves regression did not pass; preserve producing run'}
}

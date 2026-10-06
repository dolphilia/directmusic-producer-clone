[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$messages=@(& (Join-Path $PSScriptRoot 'Test-RegressionManifest.ps1') -BuildSummaryPath $BuildSummaryPath -Only 'file-output')
$messages|Write-Output
$evidence=@($messages|Where-Object {$_ -like 'Evidence: *'})
if($evidence.Count -ne 1){throw 'Exactly one new FileOutput evidence directory required'}
$result=Get-Content (Join-Path $evidence[0].Substring(10) 'run.json') -Raw|ConvertFrom-Json
if($result.buildSummary -ne [IO.Path]::GetFullPath($BuildSummaryPath)){throw 'FileOutput result candidate mismatch'}
$case=@($result.results|Where-Object id -eq 'file-output')
if($case.Count -ne 1 -or $case[0].status -ne '合格'){throw ('Source FileOutput native regression failed: '+($case|ConvertTo-Json -Compress))}

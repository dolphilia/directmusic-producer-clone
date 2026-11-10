[CmdletBinding()]
param([ValidateSet('Install','Restore')][string]$Mode='Install',[Parameter(Mandatory)][string]$TrialDirectory,
    [string]$ComparisonDirectory='work/comparison/tempo-dll/20261002T104031478Z')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected prepared trial'}
$planPath=Join-Path $trial 'plan.json'
$plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
if($plan.trial -ine $trial -or $plan.app -ine (Join-Path $trial 'app')){throw 'Trial plan mismatch'}
if(@(Get-Process -Name DMUSProd -ErrorAction SilentlyContinue|Where-Object {$_.Path -ieq (Join-Path $plan.app 'DMUSProd.exe') -or $_.Path -ieq (Join-Path $plan.launchApp 'DMUSProd.exe')})){throw 'Close trial Producer first'}
$target=Join-Path $plan.app 'TempoStripMgr.dll'
$original='bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95'
$expected='00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f'
$record=Join-Path $trial 'tempo-selection-replacement.json'
$backup=Join-Path $trial 'backup/TempoSelectionOriginal.dll'
$candidateSnapshot=Join-Path $trial 'backup/TempoSelectionCandidate.dll'
function Hash([string]$path){(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()}
if($Mode -eq 'Restore'){
    $state=Get-Content -LiteralPath $record -Raw|ConvertFrom-Json
    if($state.trial -ine $trial -or $state.planSha256 -ne (Hash $planPath) -or $state.originalSha256 -ne $original -or $state.candidateSha256 -ne $expected){throw 'Ownership mismatch'}
    if((Hash $backup) -ne $original -or (Hash $target) -notin @($expected,$original)){throw 'Unrecorded module change'}
    $restorePath=Join-Path $trial 'tempo-selection-restoration.json'
    if(Test-Path -LiteralPath $restorePath){throw 'Do not overwrite restoration evidence'}
    Copy-Item -LiteralPath $backup -Destination $target -Force
    if((Hash $target) -ne $original){throw 'Restoration failed'}
    [ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');originalSha256=$original;restored=$true;replacementSha256=(Hash $record)}|ConvertTo-Json|Set-Content -LiteralPath $restorePath -Encoding utf8
    Write-Output 'Original Tempo DLL restored after selection trial'
    return
}
if((Test-Path -LiteralPath $record) -or (Test-Path -LiteralPath $backup) -or (Test-Path -LiteralPath $candidateSnapshot)){throw 'Do not overwrite previous evidence'}
foreach($file in $plan.files){if((Hash (Join-Path $plan.app $file.name)) -ne $file.sha256){throw 'Original app changed'}}
$comparisonRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/comparison/tempo-dll'))+[IO.Path]::DirectorySeparatorChar
$comparisonDir=[IO.Path]::GetFullPath((Join-Path $repo $ComparisonDirectory))
if(-not $comparisonDir.StartsWith($comparisonRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected recorded native comparison'}
$comparisonPath=Join-Path $comparisonDir 'comparison.json'
$comparison=Get-Content -LiteralPath $comparisonPath -Raw|ConvertFrom-Json
if(-not $comparison.passed -or -not $comparison.withPageSelection -or $comparison.comparedRecords -ne 6914 -or
    @($comparison.byteChecks).Count -ne 445 -or @($comparison.copyChecks).Count -ne 123 -or @($comparison.imageChecks).Count -ne 66 -or
    @($comparison.differences).Count -ne 0 -or @($comparison.byteChecks|Where-Object {-not $_.same}).Count -ne 0 -or
    @($comparison.copyChecks|Where-Object {-not $_.sameContent}).Count -ne 0 -or @($comparison.imageChecks|Where-Object {-not $_.same}).Count -ne 0){throw 'Successful full non-clipboard regression required'}
foreach($evidence in $comparison.evidence){
    if((Hash (Join-Path $evidence.directory 'run.json')) -ne $evidence.metadataSha256 -or (Hash (Join-Path $evidence.directory 'probe.jsonl')) -ne $evidence.logSha256){throw 'Comparison evidence changed'}
}
$candidateRun=Get-Content -LiteralPath (Join-Path $comparison.candidate 'run.json') -Raw|ConvertFrom-Json
if($candidateRun.exitCode -ne 0 -or $candidateRun.dllSha256 -ne $expected){throw 'Unexpected candidate run'}
foreach($source in $candidateRun.sources){
    if($source.path.StartsWith('src/tempo/') -and (Hash (Join-Path $repo $source.path)) -ne $source.sha256){throw 'Tempo source differs from tested candidate'}
}
$candidate=Join-Path $repo 'work/build/clipboard/Release/TempoStripMgr.dll'
if((Hash $candidate) -ne $expected){throw 'Candidate binary changed'}
Copy-Item -LiteralPath $target -Destination $backup
Copy-Item -LiteralPath $candidate -Destination $candidateSnapshot
if((Hash $backup) -ne $original -or (Hash $candidateSnapshot) -ne $expected){throw 'Snapshot failed'}
$sourceDir=Join-Path $trial 'tempo-selection-trial-sources'
New-Item -ItemType Directory -Path $sourceDir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $sourceDir 'Set-TempoSelectionTrial.ps1')
$state=[ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$trial;target=$target;originalSha256=$original;candidateSha256=$expected;
    planSha256=(Hash $planPath);comparison=$comparisonPath;comparisonSha256=(Hash $comparisonPath);candidateRun=$comparison.candidate;
    candidateSnapshot=$candidateSnapshot;backup=$backup;sourceSnapshot='tempo-selection-trial-sources/Set-TempoSelectionTrial.ps1';sourceSha256=(Hash $PSCommandPath);completed=$false;
    scope='Selection/property refresh UI trial; system clipboard, audio, drag, multiple-document acceptance remain separate'}
$state|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $record -Encoding utf8
Copy-Item -LiteralPath $candidateSnapshot -Destination $target -Force
if((Hash $target) -ne $expected){throw 'Replacement failed'}
$state.completed=$true
$state|ConvertTo-Json -Depth 4|Set-Content -LiteralPath $record -Encoding utf8
Write-Output 'Tested selection-refresh candidate installed in prepared trial copy'

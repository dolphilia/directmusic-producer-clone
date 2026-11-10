[CmdletBinding()]
param([ValidateSet('Install','Restore')][string]$Mode='Install',[Parameter(Mandatory)][string]$TrialDirectory,[string]$RoundDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowed=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected prepared trial'}
$planPath=Join-Path $trial 'plan.json';$plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
if($plan.trial -ine $trial -or $plan.app -ine (Join-Path $trial 'app')){throw 'Trial plan mismatch'}
if(@(Get-Process -Name DMUSProd -ErrorAction SilentlyContinue|Where-Object {$_.Path -ieq (Join-Path $plan.app 'DMUSProd.exe') -or $_.Path -ieq (Join-Path $plan.launchApp 'DMUSProd.exe')})){throw 'Close trial Producer first'}
$original='bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95'
$candidate='00c3a3aaf7a8cbfeb2a6a0aa9c954f8421dba5e5ec75d74ee865bc6ecfd8448f'
$target=Join-Path $plan.app 'TempoStripMgr.dll'
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$roundRoot=Join-Path $trial 'tempo-host-rounds'
if($Mode -eq 'Restore'){
    $round=[IO.Path]::GetFullPath($RoundDirectory)
    if(-not $round.StartsWith($roundRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected owned host round'}
    $recordPath=Join-Path $round 'replacement.json';$record=Get-Content -LiteralPath $recordPath -Raw|ConvertFrom-Json
    if(-not $record.completed -or $record.trial -ine $trial -or $record.target -ine $target -or $record.planSha256 -ne (Hash $planPath) -or
        $record.originalSha256 -ne $original -or $record.candidateSha256 -ne $candidate -or (Hash (Join-Path $round 'OriginalTempo.dll')) -ne $original -or
        (Hash $target) -notin @($original,$candidate)){throw 'Round ownership mismatch'}
    $restorePath=Join-Path $round 'restoration.json'
    if(Test-Path -LiteralPath $restorePath){throw 'Do not overwrite restoration evidence'}
    Copy-Item -LiteralPath (Join-Path $round 'OriginalTempo.dll') -Destination $target -Force
    if((Hash $target) -ne $original){throw 'Restoration failed'}
    [ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');restored=$true;originalSha256=$original;replacementSha256=(Hash $recordPath)}|ConvertTo-Json|Set-Content -LiteralPath $restorePath -Encoding utf8
    Write-Output 'Host round original Tempo restored';return
}
if($RoundDirectory){throw 'Install allocates a fresh round'}
if(@($plan.files).Count -ne 42){throw 'Expected complete original app manifest'}
foreach($file in $plan.files){if((Hash (Join-Path $plan.app $file.name)) -ne $file.sha256){throw 'Original app file changed'}}
$priorPath=Join-Path $trial 'tempo-selection-replacement.json';$prior=Get-Content -LiteralPath $priorPath -Raw|ConvertFrom-Json
$archive=Join-Path $trial 'backup/TempoSelectionCandidate.dll'
if(-not $prior.completed -or $prior.trial -ine $trial -or $prior.planSha256 -ne (Hash $planPath) -or $prior.candidateSha256 -ne $candidate -or (Hash $archive) -ne $candidate -or
    (Hash $prior.comparison) -ne $prior.comparisonSha256){throw 'Tested candidate archive mismatch'}
$comparison=Get-Content -LiteralPath $prior.comparison -Raw|ConvertFrom-Json
if(-not $comparison.passed -or -not $comparison.withPageSelection -or $comparison.comparedRecords -ne 6914 -or @($comparison.differences).Count){throw 'Successful native comparison required'}
foreach($evidence in $comparison.evidence){if((Hash (Join-Path $evidence.directory 'run.json')) -ne $evidence.metadataSha256 -or (Hash (Join-Path $evidence.directory 'probe.jsonl')) -ne $evidence.logSha256){throw 'Native evidence changed'}}
$native=Get-Content -LiteralPath (Join-Path $comparison.candidate 'run.json') -Raw|ConvertFrom-Json
if($native.exitCode -ne 0 -or $native.dllSha256 -ne $candidate){throw 'Candidate native run failed'}
foreach($source in $native.sources){if($source.path.StartsWith('src/tempo/') -and (Hash (Join-Path $repo $source.path)) -ne $source.sha256){throw 'Tempo source differs from tested candidate'}}
$round=Join-Path $roundRoot ([DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $round|Out-Null
Copy-Item -LiteralPath $target -Destination (Join-Path $round 'OriginalTempo.dll')
Copy-Item -LiteralPath $archive -Destination (Join-Path $round 'CandidateTempo.dll')
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $round 'Set-TempoHostRound.ps1')
if((Hash (Join-Path $round 'OriginalTempo.dll')) -ne $original -or (Hash (Join-Path $round 'CandidateTempo.dll')) -ne $candidate){throw 'Round backup mismatch'}
$record=[ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$trial;target=$target;round=$round;planSha256=(Hash $planPath);
    originalSha256=$original;candidateSha256=$candidate;priorReplacementSha256=(Hash $priorPath);comparison=$prior.comparison;comparisonSha256=(Hash $prior.comparison);
    scriptSha256=(Hash $PSCommandPath);completed=$false;scope='Selection-fixed Tempo candidate host acceptance round; other original Producer modules remain dependencies'}
$recordPath=Join-Path $round 'replacement.json';$record|ConvertTo-Json|Set-Content -LiteralPath $recordPath -Encoding utf8
Copy-Item -LiteralPath (Join-Path $round 'CandidateTempo.dll') -Destination $target -Force
if((Hash $target) -ne $candidate){throw 'Replacement failed'}
$record.completed=$true;$record|ConvertTo-Json|Set-Content -LiteralPath $recordPath -Encoding utf8
Write-Output ('Round: '+$round)

[CmdletBinding()]
param([ValidateSet('Install','Restore')][string]$Mode='Install',[Parameter(Mandatory)][string]$TrialDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))
$run=[IO.Path]::GetFullPath($TrialDirectory)
if(-not $run.StartsWith($root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Use a prepared integration trial'}
$plan=Get-Content -LiteralPath (Join-Path $run 'plan.json') -Raw|ConvertFrom-Json
$app=[IO.Path]::GetFullPath($plan.app)
if($plan.trial -ine $run -or $app -ine (Join-Path $run 'app')){throw 'Trial identity mismatch'}
$active=@(Get-Process -Name DMUSProd -ErrorAction SilentlyContinue|Where-Object {$_.Path -ieq (Join-Path $app 'DMUSProd.exe') -or $_.Path -ieq (Join-Path $plan.launchApp 'DMUSProd.exe')})
if($active){throw 'Close the trial Producer before changing its DLL'}
$original=$plan.files|Where-Object name -eq 'TempoStripMgr.dll'
if(@($original).Count -ne 1){throw 'Original module identity missing'}
$target=Join-Path $app 'TempoStripMgr.dll'
$backup=Join-Path $run 'backup/TempoStripMgr.dll'
$statePath=Join-Path $run 'tempo-replacement.json'
if($Mode -eq 'Restore'){
    $state=Get-Content -LiteralPath $statePath -Raw|ConvertFrom-Json
    if($state.trial -ine $run -or $state.originalSha256 -ne $original.sha256 -or (Get-FileHash -LiteralPath (Join-Path $run 'plan.json')).Hash.ToLowerInvariant() -ne $state.planSha256){throw 'Replacement ownership changed'}
    if((Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Original backup changed'}
    $current=(Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant()
    if($current -notin @($state.candidateSha256,$original.sha256)){throw 'Trial DLL changed outside the recorded replacement'}
    Copy-Item -LiteralPath $backup -Destination $target -Force
    if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Original restoration failed'}
    [ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');originalSha256=$original.sha256;restored=$true}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'tempo-restoration.json') -Encoding utf8
    Write-Output 'Original TempoStripMgr restored'
    return
}
if(Test-Path -LiteralPath $statePath){throw 'This trial already has a replacement record'}
foreach($file in $plan.files){if((Get-FileHash -LiteralPath (Join-Path $app $file.name)).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Original app identity changed'}}
$candidate=Join-Path $repo 'work/build/probes/Release/TempoStripMgr.dll'
$expectedCandidate='9d73a68d5460571aa63e02d78b3fa65da261a35b51e14ca7a0e74159a192d12d'
if((Get-FileHash -LiteralPath $candidate).Hash.ToLowerInvariant() -ne $expectedCandidate){throw 'Candidate differs from the successful 6887-record comparison'}
New-Item -ItemType Directory -Path (Split-Path $backup -Parent) -Force|Out-Null
if(Test-Path -LiteralPath $backup){throw 'Existing backup must not be overwritten'}
Copy-Item -LiteralPath $target -Destination $backup
if((Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Backup identity mismatch'}
$state=[ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;target=$target;backup=$backup;originalSha256=$original.sha256;candidate=$candidate;candidateSha256=$expectedCandidate;planSha256=(Get-FileHash -LiteralPath (Join-Path $run 'plan.json')).Hash.ToLowerInvariant();completed=$false}
$state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
Copy-Item -LiteralPath $candidate -Destination $target -Force
if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $expectedCandidate){throw 'Replacement verification failed'}
$state.completed=$true
$state|ConvertTo-Json|Set-Content -LiteralPath $statePath -Encoding utf8
Write-Output 'Candidate TempoStripMgr installed in the trial copy'

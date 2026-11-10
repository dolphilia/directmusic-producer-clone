[CmdletBinding()]
param([ValidateSet('Install','Reinstall','Restore')][string]$Mode='Install',[Parameter(Mandatory)][string]$TrialDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$root=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))
$run=[IO.Path]::GetFullPath($TrialDirectory)
if(-not $run.StartsWith($root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Use a prepared trial'}
$planPath=Join-Path $run 'plan.json'
$plan=Get-Content -LiteralPath $planPath -Raw|ConvertFrom-Json
$app=[IO.Path]::GetFullPath($plan.app)
if($plan.trial -ine $run -or $app -ine (Join-Path $run 'app')){throw 'Trial identity changed'}
if(@(Get-Process -Name DMUSProd -ErrorAction SilentlyContinue|Where-Object {$_.Path -ieq (Join-Path $app 'DMUSProd.exe') -or $_.Path -ieq (Join-Path $plan.launchApp 'DMUSProd.exe')})){throw 'Close the trial Producer first'}
$original=$plan.files|Where-Object name -eq 'TempoStripMgr.dll'
if(@($original).Count -ne 1){throw 'Original module missing'}
$target=Join-Path $app 'TempoStripMgr.dll'
$backup=Join-Path $run 'backup/TempoClipboardOriginal.dll'
$record=Join-Path $run 'tempo-clipboard-replacement.json'
$expected='69029024df3a082e14343cc8684d2baeb9162b3087fa8f310a2d29e3b5bdd733'
if($Mode -eq 'Restore'){
 $state=Get-Content -LiteralPath $record -Raw|ConvertFrom-Json
 if($state.trial -ine $run -or $state.candidateSha256 -ne $expected -or $state.originalSha256 -ne $original.sha256 -or $state.planSha256 -ne (Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant()){throw 'Ownership record changed'}
 if((Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Backup changed'}
 if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -notin @($expected,$original.sha256)){throw 'Unrecorded target change'}
 Copy-Item -LiteralPath $backup -Destination $target -Force
 if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Restoration failed'}
 $previousRestoration=Join-Path $run 'tempo-clipboard-restoration.json'
 if(Test-Path -LiteralPath $previousRestoration){Copy-Item -LiteralPath $previousRestoration -Destination (Join-Path $run ('tempo-clipboard-restoration-history-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'.json'))}
 [ordered]@{restoredUtc=[DateTime]::UtcNow.ToString('o');restored=$true;originalSha256=$original.sha256}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'tempo-clipboard-restoration.json') -Encoding utf8
 return
}
if($Mode -eq 'Install' -and ((Test-Path -LiteralPath $record) -or (Test-Path -LiteralPath $backup))){throw 'Do not overwrite previous trial evidence'}
foreach($file in $plan.files){if((Get-FileHash -LiteralPath (Join-Path $app $file.name)).Hash.ToLowerInvariant() -ne $file.sha256){throw 'Original app changed'}}
$candidate=Join-Path $repo 'work/build/clipboard/Release/TempoStripMgr.dll'
if((Get-FileHash -LiteralPath $candidate).Hash.ToLowerInvariant() -ne $expected){throw 'Candidate changed'}
$comparison=Join-Path $repo 'work/build/clipboard/feature-comparison.json'
$feature=Get-Content -LiteralPath $comparison -Raw|ConvertFrom-Json
if(-not $feature.featureCasesMatch -or $feature.candidateSha256 -ne $expected -or @($feature.cases).Count -ne 4){throw 'Limited feature comparison missing'}
if($Mode -eq 'Reinstall'){
 $prior=Get-Content -LiteralPath $record -Raw|ConvertFrom-Json
 $restored=Get-Content -LiteralPath (Join-Path $run 'tempo-clipboard-restoration.json') -Raw|ConvertFrom-Json
 if(-not $prior.completed -or $prior.trial -ine $run -or $prior.candidateSha256 -ne $expected -or $prior.originalSha256 -ne $original.sha256 -or $prior.planSha256 -ne (Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant() -or -not $restored.restored -or $restored.originalSha256 -ne $original.sha256){throw 'Prior replacement/restoration ownership mismatch'}
 if((Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $original.sha256 -or $prior.featureComparisonSha256 -ne (Get-FileHash -LiteralPath $comparison).Hash.ToLowerInvariant()){throw 'Prior evidence changed'}
 $repeatRecord=Join-Path $run ('tempo-clipboard-reinstall-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'.json')
 $repeat=[ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;originalSha256=$original.sha256;candidateSha256=$expected;priorReplacementSha256=(Get-FileHash -LiteralPath $record).Hash.ToLowerInvariant();priorRestorationSha256=(Get-FileHash -LiteralPath (Join-Path $run 'tempo-clipboard-restoration.json')).Hash.ToLowerInvariant();completed=$false}
 $repeat|ConvertTo-Json|Set-Content -LiteralPath $repeatRecord -Encoding utf8
 Copy-Item -LiteralPath $candidate -Destination $target -Force
 if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $expected){throw 'Repeated replacement failed'}
 $repeat.completed=$true
 $repeat|ConvertTo-Json|Set-Content -LiteralPath $repeatRecord -Encoding utf8
 Write-Output ('Repeated replacement evidence: '+$repeatRecord)
 return
}
Copy-Item -LiteralPath $target -Destination $backup
if((Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $original.sha256){throw 'Backup mismatch'}
$state=[ordered]@{startedUtc=[DateTime]::UtcNow.ToString('o');trial=$run;originalSha256=$original.sha256;candidateSha256=$expected;planSha256=(Get-FileHash -LiteralPath $planPath).Hash.ToLowerInvariant();featureComparison=$comparison;featureComparisonSha256=(Get-FileHash -LiteralPath $comparison).Hash.ToLowerInvariant();fullNativeRunsPassed=$feature.fullRunsPassed;completed=$false}
$state|ConvertTo-Json|Set-Content -LiteralPath $record -Encoding utf8
Copy-Item -LiteralPath $candidate -Destination $target -Force
if((Get-FileHash -LiteralPath $target).Hash.ToLowerInvariant() -ne $expected){throw 'Replacement failed'}
$state.completed=$true
$state|ConvertTo-Json|Set-Content -LiteralPath $record -Encoding utf8

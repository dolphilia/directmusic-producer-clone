#Requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Destination)
. (Join-Path $PSScriptRoot 'SourceProductDeployment.ps1')
$summaryPath=(Resolve-Path -LiteralPath $BuildSummaryPath).Path;$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged -or $summary.configureExitCode -ne 0 -or $summary.buildExitCode -ne 0 -or $summary.installExitCode -ne 0 -or $summary.originalModulesRequired){throw 'Source-only successful build required'}
$cache=Get-Content -LiteralPath (Join-Path $summary.buildDirectory 'CMakeCache.txt') -Raw
if($cache -notmatch '(?m)^PRODUCER_BUILD_REFERENCE_TOOLS:BOOL=OFF\r?$'){throw 'Reference tools must be OFF'}
foreach($s in $summary.sources){$p=Join-Path $summary.sourceRoot $s.path;if((Get-ProductHash $p) -ne $s.sha256){throw ('Changed saved source: '+$s.path)}}
$source=Get-PlainProductPath $summary.installDirectory;Assert-ProductTree $source $ProductFiles
$dest=Get-PlainProductPath $Destination
if(Test-Path -LiteralPath $dest){throw 'Destination must not exist'}
$parent=[IO.Path]::GetDirectoryName($dest);if(-not(Test-Path -LiteralPath $parent -PathType Container)){throw 'Destination parent must exist'}
$assets=@()
foreach($rel in $ProductFiles){$p=Get-ProductChild $source $rel;$hash=Get-ProductHash $p
 if($rel -eq 'bin/Producer.exe'){$expected=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($expected.Count -ne 1 -or $hash -ne $expected[0].sha256){throw 'Installed Producer hash mismatch'}}
 else{$sourceRel=if($rel.StartsWith('tools/')){'scripts/'+$rel.Substring(6)}else{$rel};$expected=@($summary.sources|Where-Object path -eq $sourceRel);if($expected.Count -ne 1 -or $hash -ne $expected[0].sha256){throw ('Packaged source hash mismatch: '+$rel)}}
 $assets+=[ordered]@{path=$rel;sha256=$hash;bytes=(Get-Item -LiteralPath $p).Length}}
# Publish only a fully verified directory. A failed staging tree is evidence,
# never an installation at Destination. The existing-destination check above
# remains, and Directory.Move also refuses a concurrently created destination.
$stage=Get-PlainProductPath (Join-Path $parent ('.source-product-stage-'+[Guid]::NewGuid().ToString('N')))
New-Item -ItemType Directory -Path $stage|Out-Null
try{foreach($asset in $assets){$p=Get-ProductChild $stage $asset.path;New-Item -ItemType Directory -Path (Split-Path $p -Parent) -Force|Out-Null;Copy-Item -LiteralPath (Get-ProductChild $source $asset.path) -Destination $p;if((Get-ProductHash $p) -ne $asset.sha256){throw 'Copied asset hash mismatch'}}
 $manifest=[ordered]@{schema=1;owner=$ProductOwner;root=$dest;createdUtc=[DateTime]::UtcNow.ToString('o');candidate=(Split-Path (Split-Path $summaryPath -Parent) -Leaf);buildSummarySha256=(Get-ProductHash $summaryPath);files=$assets;registryChanges=@();externalRuntimeRemoved=$false}
 $manifestPath=Join-Path $stage 'source-product-install.json'
 $manifest|ConvertTo-Json -Depth 6|Set-Content -LiteralPath $manifestPath -Encoding utf8
 Assert-ProductTree $stage (@($ProductFiles)+@('source-product-install.json'))
 foreach($asset in $assets){if((Get-ProductHash (Get-ProductChild $stage $asset.path)) -ne $asset.sha256){throw 'Staged asset changed before publication'}}
 $readback=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
 if($readback.owner -ne $ProductOwner -or $readback.root -ne $dest -or $readback.files.Count -ne $assets.Count){throw 'Staged manifest readback mismatch'}
 $manifestHash=Get-ProductHash $manifestPath
 $null=Get-PlainProductPath $stage;$null=Get-PlainProductPath $dest
 [IO.Directory]::Move($stage,$dest)
 [ordered]@{passed=$true;destination=$dest;manifestSha256=$manifestHash;files=$assets;publication='Verified staging then same-parent directory rename';scope='Owned source files only; GUI/audio/Q2 acceptance separate'}
}catch{throw ('Install not published; retained staging for inspection: '+$stage+'; intended destination='+$dest+'. '+$_.Exception.Message)}

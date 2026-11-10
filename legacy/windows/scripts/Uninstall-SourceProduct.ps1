#Requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$InstallRoot)
. (Join-Path $PSScriptRoot 'SourceProductDeployment.ps1')
$root=Get-PlainProductPath $InstallRoot;$manifestPath=Join-Path $root 'source-product-install.json';$manifest=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
if($manifest.schema -ne 1 -or $manifest.owner -ne $ProductOwner -or -not $root.Equals($manifest.root,[StringComparison]::OrdinalIgnoreCase)){throw 'Manifest owner/root mismatch'}
if($manifest.files.Count -ne $ProductFiles.Count){throw 'Manifest inventory mismatch'}
$seen=@{}
foreach($asset in $manifest.files){$p=Get-ProductChild $root $asset.path;if($seen.ContainsKey($asset.path)){throw 'Duplicate manifest file'};$seen[$asset.path]=$true;if(-not(Test-Path -LiteralPath $p -PathType Leaf) -or (Get-ProductHash $p) -ne $asset.sha256 -or (Get-Item -LiteralPath $p).Length -ne $asset.bytes){throw ('Missing/modified owned file; unchanged installation retained: '+$asset.path)}}
Assert-ProductTree $root (@($ProductFiles)+@('source-product-install.json'))
$removed=@()
try{foreach($rel in $ProductFiles){$p=Get-ProductChild $root $rel;Remove-Item -LiteralPath $p -ErrorAction Stop;$removed+=$rel};Remove-Item -LiteralPath $manifestPath -ErrorAction Stop;$removed+='source-product-install.json'
 foreach($dir in @('bin','tools','docs')){$p=Get-PlainProductPath (Join-Path $root $dir);if(@(Get-ChildItem -LiteralPath $p -Force).Count -ne 0){throw 'Directory changed during removal'};Remove-Item -LiteralPath $p -ErrorAction Stop}
 if(@(Get-ChildItem -LiteralPath $root -Force).Count -ne 0){throw 'Root changed during removal'};Remove-Item -LiteralPath $root -ErrorAction Stop
 [ordered]@{passed=$true;root=$root;removed=$removed;registryChanged=$false;externalRuntimeRemoved=$false}
}catch{throw ('Uninstall I/O failed; do not count success. Removed='+($removed -join ',')+'; retained root='+$root+'; '+$_.Exception.Message)}

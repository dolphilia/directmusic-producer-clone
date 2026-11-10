#Requires -Version 7.2
[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=(Resolve-Path -LiteralPath $BuildSummaryPath).Path;$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
$run=Join-Path $repo ('work/acceptance/source-deployment/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
$package=$summary.installDirectory;$installer=Join-Path $package 'tools/Install-SourceProduct.ps1';$uninstaller=Join-Path $package 'tools/Uninstall-SourceProduct.ps1'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
function Tree([string]$p){@((Get-ChildItem -LiteralPath $p -Force -Recurse|Sort-Object FullName)|ForEach-Object {[ordered]@{path=[IO.Path]::GetRelativePath($p,$_.FullName);directory=$_.PSIsContainer;sha256=if($_.PSIsContainer){$null}else{Hash $_.FullName}}})|ConvertTo-Json -Depth 4 -Compress}
$results=[Collections.Generic.List[object]]::new()
. (Join-Path $package 'tools/SourceProductDeployment.ps1')
foreach($tag in @([uint32]0,[uint32]2415919130,[uint32]2415947802,[uint32]2415980570)){if(-not(Test-ProductReparseTag $tag)){throw 'Ordinary/cloud reparse tag rejected'}}
foreach($tag in @([uint32]2684354563,[uint32]2684354572,[uint32]2147483733)){if(Test-ProductReparseTag $tag){throw 'Redirecting/unknown reparse tag accepted'}}
$results.Add([ordered]@{name='reparse tag policy: normal/cloud accepted; junction/symlink/unknown rejected';passed=$true;cloudAncestorTag=if(Test-Path -LiteralPath (Join-Path $env:USERPROFILE 'OneDrive')){([SourceProduct.PathMetadata]::Tag((Join-Path $env:USERPROFILE 'OneDrive'))).ToString('x8')}else{$null}})
function Expect-Refusal([string]$Name,[scriptblock]$Action,[string]$Directory){$before=Tree $Directory;$rejected=$false;$message=$null;try{& $Action|Out-Null}catch{$rejected=$true;$message=$_.Exception.Message};$after=Tree $Directory;if(-not $rejected -or $before -ne $after){throw ('Refusal must retain every file: '+$Name)};$results.Add([ordered]@{name=$Name;passed=$true;rejected=$true;reason=$message;unchanged=$true})}
# Isolated command fault injection exercises the actual packaged installer.
# It changes no OS policy and leaves both failed staging trees for inspection.
$faultRunner=Join-Path $run 'install-fault-runner.ps1'
@'
param([string]$Installer,[string]$Summary,[string]$Destination,[string]$Fault)
$ErrorActionPreference='Stop';$global:productInstallFaultCopies=0;$global:productInstallFault=$Fault
function Copy-Item {
 param([string]$LiteralPath,[string]$Destination)
 $global:productInstallFaultCopies++
 if($global:productInstallFault -eq 'middle-copy' -and $global:productInstallFaultCopies -eq 2){throw 'Injected second asset copy I/O failure'}
 Microsoft.PowerShell.Management\Copy-Item -LiteralPath $LiteralPath -Destination $Destination
}
function Set-Content {
 [CmdletBinding()]param([Parameter(ValueFromPipeline)]$Value,[string]$LiteralPath,[string]$Encoding)
 process{if($global:productInstallFault -eq 'manifest-write' -and $LiteralPath.EndsWith('source-product-install.json')){throw 'Injected manifest publication I/O failure'};Microsoft.PowerShell.Management\Set-Content -LiteralPath $LiteralPath -Value $Value -Encoding $Encoding}
}
try{& $Installer -BuildSummaryPath $Summary -Destination $Destination|Out-Null;throw 'Fault not exercised'}catch{if($_.Exception.Message -notmatch 'Install not published; retained staging'){throw};Write-Output $_.Exception.Message}
'@|Set-Content -LiteralPath $faultRunner -Encoding utf8
foreach($fault in @('middle-copy','manifest-write')){
 $faultRoot=Join-Path $run $fault;New-Item -ItemType Directory -Path $faultRoot|Out-Null
 $faultDest=Join-Path $faultRoot 'MustNotBePublished';$beforePackage=Tree $package
 $message=& (Join-Path $PSHOME 'pwsh.exe') -NoProfile -File $faultRunner -Installer $installer -Summary $summaryPath -Destination $faultDest -Fault $fault
 if($LASTEXITCODE -ne 0 -or (Test-Path -LiteralPath $faultDest) -or (Tree $package) -ne $beforePackage){throw ('Failed install must leave destination absent and package unchanged: '+$fault)}
 $stages=@(Get-ChildItem -LiteralPath $faultRoot -Directory -Force)
 if($stages.Count -ne 1 -or $stages[0].Name -notmatch '^\.source-product-stage-[0-9a-f]{32}$'){throw 'Expected retained failed staging tree'}
 $copied=@(Get-ChildItem -LiteralPath $stages[0].FullName -File -Recurse)
 $expectedCopies=if($fault -eq 'middle-copy'){1}else{$ProductFiles.Count}
 if($copied.Count -ne $expectedCopies){throw 'Fault did not occur at expected transaction stage'}
 [ordered]@{fault=$fault;destinationAbsent=$true;packageUnchanged=$true;retainedStaging=$stages[0].FullName;copiedAssets=$copied.Count;message=$message}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $faultRoot 'fault-proof.json') -Encoding utf8
 $results.Add([ordered]@{name=('atomic install '+$fault);passed=$true;destinationAbsent=$true;packageUnchanged=$true;stagingRetained=$true;evidence=(Join-Path $faultRoot 'fault-proof.json')})
}
$own=Join-Path $run 'Owned';$install=& $installer -BuildSummaryPath $summaryPath -Destination $own
$install|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'installed.json') -Encoding utf8
Expect-Refusal 'existing destination' {& $installer -BuildSummaryPath $summaryPath -Destination $own} $own
$doc=Join-Path $own 'user-document.sgp';[IO.File]::WriteAllText($doc,'user-authored sentinel')
Expect-Refusal 'unlisted user document' {& $uninstaller -InstallRoot $own} $own
Move-Item -LiteralPath $doc -Destination (Join-Path $run 'retained-user-document.sgp')
$asset=Join-Path $own 'docs/source-product-deployment.md';$original=[IO.File]::ReadAllBytes($asset);[IO.File]::AppendAllText($asset,'modified sentinel')
Expect-Refusal 'modified owned asset' {& $uninstaller -InstallRoot $own} $own
[IO.File]::WriteAllBytes($asset,$original)
$holding=Join-Path $run 'held-owned-readme';Move-Item -LiteralPath $asset -Destination $holding
Expect-Refusal 'missing owned asset' {& $uninstaller -InstallRoot $own} $own
Move-Item -LiteralPath $holding -Destination $asset
$manifest=Join-Path $own 'source-product-install.json';$bytes=[IO.File]::ReadAllBytes($manifest);$m=Get-Content -LiteralPath $manifest -Raw|ConvertFrom-Json;$m.root=$run;$m|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $manifest -Encoding utf8
Expect-Refusal 'manifest root mismatch' {& $uninstaller -InstallRoot $own} $own
[IO.File]::WriteAllBytes($manifest,$bytes)
$m=Get-Content -LiteralPath $manifest -Raw|ConvertFrom-Json;$m.files[0].path='../outside.exe';$m|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $manifest -Encoding utf8
Expect-Refusal 'manifest traversal' {& $uninstaller -InstallRoot $own} $own
[IO.File]::WriteAllBytes($manifest,$bytes)
$m=Get-Content -LiteralPath $manifest -Raw|ConvertFrom-Json;$m.files[1].path=$m.files[0].path;$m|ConvertTo-Json -Depth 7|Set-Content -LiteralPath $manifest -Encoding utf8
Expect-Refusal 'duplicate inventory' {& $uninstaller -InstallRoot $own} $own
[IO.File]::WriteAllBytes($manifest,$bytes)
$empty=Join-Path $own 'User';New-Item -ItemType Directory -Path $empty|Out-Null
Expect-Refusal 'unlisted directory' {& $uninstaller -InstallRoot $own} $own
Remove-Item -LiteralPath $empty
$smokeOut=Join-Path $run 'smoke.stdout.txt';$smokeErr=Join-Path $run 'smoke.stderr.txt'
$p=Start-Process -FilePath (Join-Path $own 'bin/Producer.exe') -ArgumentList @('--smoke',('"'+(Join-Path $run 'host-smoke')+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput $smokeOut -RedirectStandardError $smokeErr
if(-not $p.WaitForExit(60000)){throw 'Installed smoke timed out; process retained, no uninstall'};$p.Refresh();if($p.ExitCode -ne 0){throw 'Installed smoke failed'};$results.Add([ordered]@{name='installed source smoke';passed=$true;exitCode=$p.ExitCode})
$removed=& $uninstaller -InstallRoot $own;if(Test-Path -LiteralPath $own){throw 'Owned installation still exists'};if([IO.File]::ReadAllText((Join-Path $run 'retained-user-document.sgp')) -ne 'user-authored sentinel'){throw 'User sentinel changed'}
$removed|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'uninstalled.json') -Encoding utf8
$results.Add([ordered]@{name='verified owned uninstall';passed=$true;directoryRemoved=$true;userDocumentRetained=$true})
$copied=Join-Path $run 'TamperedPackage';New-Item -ItemType Directory -Path $copied|Out-Null
Copy-Item -LiteralPath (Join-Path $package 'bin') -Destination $copied -Recurse;Copy-Item -LiteralPath (Join-Path $package 'tools') -Destination $copied -Recurse;Copy-Item -LiteralPath (Join-Path $package 'docs') -Destination $copied -Recurse
[IO.File]::AppendAllText((Join-Path $copied 'docs/source-product-deployment.md'),'tampered input sentinel')
$copiedSummary=Join-Path $run 'tampered-package-summary.json';$testSummary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json;$testSummary.installDirectory=$copied;$testSummary|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $copiedSummary -Encoding utf8
$forbidden=Join-Path $run 'MustNotExist'
Expect-Refusal 'modified source package' {& $installer -BuildSummaryPath $copiedSummary -Destination $forbidden} $copied
if(Test-Path -LiteralPath $forbidden){throw 'Tampered input created destination'}
$gui=Join-Path $run 'GuiInstall';$guiInstall=& $installer -BuildSummaryPath $summaryPath -Destination $gui
$guiInstall|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'gui-installed.json') -Encoding utf8
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');candidate=(Split-Path (Split-Path $summaryPath -Parent) -Leaf);buildSummary=$summaryPath;buildSummarySha256=(Hash $summaryPath);driverSha256=(Hash $PSCommandPath);installerSha256=(Hash $installer);uninstallerSha256=(Hash $uninstaller);exeSha256=(Hash (Join-Path $gui 'bin/Producer.exe'));passed=$true;checks=$results.Count;results=@($results.ToArray());guiInstall=$gui;guiAcceptance='unexecuted';originalComparison='blocked';scope='Source installation/refusal/uninstall/smoke; all40/all8 and independent Q2 separate';fullAcceptance=$false}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$run)

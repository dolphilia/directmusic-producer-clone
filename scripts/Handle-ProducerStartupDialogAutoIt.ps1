[CmdletBinding()]
param([Parameter(Mandatory)][string]$TrialDirectory,[Parameter(Mandatory)][int]$ProcessId,[switch]$Click)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowedRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected a prepared integration trial'}
$plan=Get-Content -LiteralPath (Join-Path $trial 'plan.json') -Raw | ConvertFrom-Json
$exe=Join-Path $trial 'app/DMUSProd.exe'
$exeHash='fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011'
if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne $exeHash){throw 'Producer EXE identity mismatch'}
if([IO.Path]::GetFullPath($plan.app) -ne [IO.Path]::GetFullPath((Join-Path $trial 'app'))){throw 'Trial plan path mismatch'}
$target=Get-Process -Id $ProcessId
$executionIdentity=[Security.Principal.WindowsIdentity]::GetCurrent()
$desktopIdentity=[ordered]@{executionUser=$executionIdentity.Name;executionSid=$executionIdentity.User.Value;executionSession=(Get-Process -Id $PID).SessionId;producerSession=$target.SessionId}
if(-not [string]::Equals($target.Path,$exe,[StringComparison]::OrdinalIgnoreCase)){throw 'Target process path mismatch'}
$toolRoot=Join-Path $repo 'work/tools/autoit-3.3.18.0'
$tool=Join-Path $toolRoot 'portable/install/AutoIt3.exe'
$toolHash='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'
$archiveHash='ceb666a993a9f62621c3a0d4ee602774b2e7f543de1f08ec0380632ee3f89beb'
if((Get-FileHash -LiteralPath $tool).Hash.ToLowerInvariant() -ne $toolHash){throw 'AutoIt interpreter identity mismatch'}
if((Get-FileHash -LiteralPath (Join-Path $toolRoot 'autoit-v3.zip')).Hash.ToLowerInvariant() -ne $archiveHash){throw 'AutoIt archive identity mismatch'}
$signature=Get-AuthenticodeSignature -LiteralPath $tool
if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'CN=AUTOIT CONSULTING LTD,'){throw 'Expected valid AutoIt publisher signature'}
$script=Join-Path $repo 'scripts/ProducerStartupDialog.au3'
$mode=if($Click){'click'}else{'observe'}
$run=Join-Path $trial ('startup-autoit-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'-'+$mode)
New-Item -ItemType Directory -Path $run | Out-Null
$sourcePaths=@('scripts/ProducerStartupDialog.au3','scripts/Handle-ProducerStartupDialogAutoIt.ps1')
$sourceHashes=@($sourcePaths|ForEach-Object{[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash.ToLowerInvariant()}})
foreach($source in $sourceHashes){$destination=Join-Path $run ('sources/'+$source.path);New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null;Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $destination;if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Source snapshot changed'}}
$libraryHashes=@(Get-ChildItem -LiteralPath (Join-Path $toolRoot 'portable/install/Include') -Filter '*.au3' -File | Sort-Object Name | ForEach-Object{[ordered]@{name=$_.Name;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$arguments=@('/ErrorStdOut',('"{0}"' -f $script),[string]$ProcessId,('"{0}"' -f $exe),('"{0}"' -f (Join-Path $run 'dialog.png')),$mode,('"{0}"' -f (Join-Path $run 'dialog.jsonl')))
$exitCode=$null;$launchError=$null;$timedOut=$false
try{
  $toolProcess=Start-Process -FilePath $tool -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
  $timedOut=-not $toolProcess.WaitForExit(15000)
  if($timedOut){Stop-Process -Id $toolProcess.Id;$toolProcess.WaitForExit()}
  $toolProcess.Refresh();$exitCode=$toolProcess.ExitCode
}catch{$launchError=$_.Exception.Message}
[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');pid=$ProcessId;mode=$mode;desktopIdentity=$desktopIdentity;exe=$exe;exeSha256=$exeHash;tool=$tool;toolVersion='3.3.18.0';toolSha256=$toolHash;downloadUrl='https://www.autoitscript.com/files/autoit3/autoit-v3.zip';archiveSha256=$archiveHash;signature=[string]$signature.Status;signer=$signature.SignerCertificate.Subject;exitCode=$exitCode;launchError=$launchError;timedOut=$timedOut;sources=$sourceHashes;sourceSnapshot='sources';libraries=$libraryHashes;scope='Known Producer startup dialog only; official portable interpreter; no installation, registry or security changes'} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
foreach($log in @('dialog.jsonl','stdout.txt','stderr.txt')){if(Test-Path -LiteralPath (Join-Path $run $log)){Get-Content -LiteralPath (Join-Path $run $log)}}
if($launchError -or $timedOut -or $exitCode -ne 0){throw 'AutoIt startup dialog operation failed; inspect retained evidence'}

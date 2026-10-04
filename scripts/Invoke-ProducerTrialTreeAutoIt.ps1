[CmdletBinding()]
param([Parameter(Mandatory)][string]$TrialDirectory,[Parameter(Mandatory)][int]$ProcessId,[Parameter(Mandatory)][string]$Item,[Parameter(Mandatory)][string]$ExpectedText,[ValidateSet('observe','expand','open','properties')][string]$Action='observe')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowedRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected prepared trial'}
$plan=Get-Content -LiteralPath (Join-Path $trial 'plan.json') -Raw|ConvertFrom-Json
if($plan.trial -ine $trial -or $plan.app -ine (Join-Path $trial 'app')){throw 'Trial plan changed'}
$exe=Join-Path $trial 'app/DMUSProd.exe'
if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne 'fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011' -or (Get-Process -Id $ProcessId).Path -ine $exe){throw 'Producer identity mismatch'}
if($Item -notmatch '^#\d+(\|#\d+)*$'){throw 'Use explicit observed tree indices'}
$tool=Join-Path $repo 'work/tools/autoit-3.3.18.0/portable/install/AutoIt3.exe'
$toolHash='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'
$signature=Get-AuthenticodeSignature -LiteralPath $tool
if((Get-FileHash -LiteralPath $tool).Hash.ToLowerInvariant() -ne $toolHash -or $signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'CN=AUTOIT CONSULTING LTD,'){throw 'Expected official signed AutoIt interpreter'}
$run=Join-Path $trial ('tree-autoit-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'-'+$Action)
New-Item -ItemType Directory -Path $run|Out-Null
$script=Join-Path $repo 'scripts/Inspect-ProducerTrialTree.au3'
$sources=@('scripts/Inspect-ProducerTrialTree.au3','scripts/Invoke-ProducerTrialTreeAutoIt.ps1')|ForEach-Object{[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash.ToLowerInvariant()}}
foreach($source in $sources){$destination=Join-Path $run ('sources/'+$source.path);New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force|Out-Null;Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $destination;if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Source snapshot changed'}}
$arguments=@('/ErrorStdOut',('"{0}"' -f $script),[string]$ProcessId,('"{0}"' -f $exe),('"{0}"' -f $Item),('"{0}"' -f $ExpectedText),$Action,('"{0}"' -f (Join-Path $run 'tree.jsonl')))
$exitCode=$null;$launchError=$null;$timedOut=$false
$executionIdentity=[Security.Principal.WindowsIdentity]::GetCurrent()
$desktopIdentity=[ordered]@{executionUser=$executionIdentity.Name;executionSid=$executionIdentity.User.Value;executionSession=(Get-Process -Id $PID).SessionId;producerSession=(Get-Process -Id $ProcessId).SessionId}
try{$operation=Start-Process -FilePath $tool -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt');$timedOut=-not $operation.WaitForExit(15000);if($timedOut){Stop-Process -Id $operation.Id;$operation.WaitForExit()};$operation.Refresh();$exitCode=$operation.ExitCode}catch{$launchError=$_.Exception.Message}
[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');pid=$ProcessId;action=$Action;desktopIdentity=$desktopIdentity;item=$Item;expectedText=$ExpectedText;toolSha256=$toolHash;signature=[string]$signature.Status;exitCode=$exitCode;launchError=$launchError;timedOut=$timedOut;sources=@($sources);sourceSnapshot='sources';scope='Prepared Producer trial tree navigation only'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$run)
foreach($log in @('tree.jsonl','stdout.txt','stderr.txt')){if(Test-Path -LiteralPath (Join-Path $run $log)){Get-Content -LiteralPath (Join-Path $run $log)}}
if($launchError -or $timedOut -or $exitCode -ne 0){throw 'AutoIt tree operation failed; inspect retained evidence'}

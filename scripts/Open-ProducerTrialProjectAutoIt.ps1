[CmdletBinding()]
param([Parameter(Mandatory)][string]$TrialDirectory,[Parameter(Mandatory)][int]$ProcessId,[Parameter(Mandatory)][string]$ProjectRelativePath,[switch]$Open)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowedRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected prepared trial'}
$plan=Get-Content -LiteralPath (Join-Path $trial 'plan.json') -Raw|ConvertFrom-Json
if($plan.trial -ine $trial -or $plan.app -ine (Join-Path $trial 'app')){throw 'Trial plan changed'}
$exe=Join-Path $trial 'app/DMUSProd.exe'
if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne 'fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011' -or (Get-Process -Id $ProcessId).Path -ine $exe){throw 'Producer identity mismatch'}
$projectRoot=[IO.Path]::GetFullPath((Join-Path $trial 'app/UiTest'))+[IO.Path]::DirectorySeparatorChar
$project=[IO.Path]::GetFullPath((Join-Path $projectRoot $ProjectRelativePath))
if(-not $project.StartsWith($projectRoot,[StringComparison]::OrdinalIgnoreCase) -or [IO.Path]::GetExtension($project) -ine '.pro'){throw 'Expected trial UiTest project'}
$projectHash=(Get-FileHash -LiteralPath $project).Hash.ToLowerInvariant()
$tool=Join-Path $repo 'work/tools/autoit-3.3.18.0/portable/install/AutoIt3.exe'
$toolHash='bdd2b7236a110b04c288380ad56e8d7909411da93eed2921301206de0cb0dda1'
$signature=Get-AuthenticodeSignature -LiteralPath $tool
if((Get-FileHash -LiteralPath $tool).Hash.ToLowerInvariant() -ne $toolHash -or $signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'CN=AUTOIT CONSULTING LTD,'){throw 'Expected official signed AutoIt interpreter'}
$mode=if($Open){'open'}else{'observe'}
$run=Join-Path $trial ('project-autoit-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'-'+$mode)
New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $project -Destination (Join-Path $run 'project-before.pro')
if((Get-FileHash -LiteralPath (Join-Path $run 'project-before.pro')).Hash.ToLowerInvariant() -ne $projectHash){throw 'Project input snapshot changed'}
$script=Join-Path $repo 'scripts/Open-ProducerTrialProject.au3'
$sources=@('scripts/Open-ProducerTrialProject.au3','scripts/Open-ProducerTrialProjectAutoIt.ps1')|ForEach-Object{[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash.ToLowerInvariant()}}
foreach($source in $sources){$destination=Join-Path $run ('sources/'+$source.path);New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force|Out-Null;Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $destination;if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Source snapshot changed'}}
$arguments=@('/ErrorStdOut',('"{0}"' -f $script),[string]$ProcessId,('"{0}"' -f $exe),('"{0}"' -f $project),$mode,('"{0}"' -f (Join-Path $run 'dialog.png')),('"{0}"' -f (Join-Path $run 'dialog.jsonl')))
$exitCode=$null;$launchError=$null;$timedOut=$false
$executionIdentity=[Security.Principal.WindowsIdentity]::GetCurrent()
$desktopIdentity=[ordered]@{executionUser=$executionIdentity.Name;executionSid=$executionIdentity.User.Value;executionSession=(Get-Process -Id $PID).SessionId;producerSession=(Get-Process -Id $ProcessId).SessionId}
try{$operation=Start-Process -FilePath $tool -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt');$timedOut=-not $operation.WaitForExit(15000);if($timedOut){Stop-Process -Id $operation.Id;$operation.WaitForExit()};$operation.Refresh();$exitCode=$operation.ExitCode}catch{$launchError=$_.Exception.Message}
[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');pid=$ProcessId;mode=$mode;desktopIdentity=$desktopIdentity;project=$project;projectSha256=$projectHash;projectBeforeSnapshot='project-before.pro';projectAfterSha256=(Get-FileHash -LiteralPath $project).Hash.ToLowerInvariant();toolSha256=$toolHash;signature=[string]$signature.Status;exitCode=$exitCode;launchError=$launchError;timedOut=$timedOut;sources=@($sources);sourceSnapshot='sources';scope='Open prepared trial UiTest project only'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$run)
foreach($log in @('dialog.jsonl','stdout.txt','stderr.txt')){if(Test-Path -LiteralPath (Join-Path $run $log)){Get-Content -LiteralPath (Join-Path $run $log)}}
if($launchError -or $timedOut -or $exitCode -ne 0){throw 'AutoIt project operation failed; inspect retained evidence'}

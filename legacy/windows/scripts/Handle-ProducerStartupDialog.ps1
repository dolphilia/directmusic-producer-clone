[CmdletBinding()]
param([Parameter(Mandatory)][string]$TrialDirectory,[Parameter(Mandatory)][int]$ProcessId,[switch]$Click)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$trial=[IO.Path]::GetFullPath($TrialDirectory)
$allowedRoot=[IO.Path]::GetFullPath((Join-Path $repo 'work/integration/user-trial'))+[IO.Path]::DirectorySeparatorChar
if(-not $trial.StartsWith($allowedRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Expected a prepared integration trial'}
$plan=Get-Content -LiteralPath (Join-Path $trial 'plan.json') -Raw | ConvertFrom-Json
$exe=Join-Path $trial 'app/DMUSProd.exe'
$expected='fad2eec4d5dacd3bfd67694ea63ca169517902b73e30d283998cf7726e41c011'
if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne $expected){throw 'Producer EXE identity mismatch'}
if([IO.Path]::GetFullPath($plan.app) -ne [IO.Path]::GetFullPath((Join-Path $trial 'app'))){throw 'Trial plan path mismatch'}
$helper=Join-Path $repo 'work/build/startup-dialog/Release/producer_startup_dialog.exe'
$mode=if($Click){'click'}else{'observe'}
$run=Join-Path $trial ('startup-dialog-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')+'-'+$mode)
New-Item -ItemType Directory -Path $run | Out-Null
$sourcePaths=@('tests/native/producer_startup_dialog.cpp','tests/native/startup_dialog/CMakeLists.txt','scripts/Handle-ProducerStartupDialog.ps1')
$sourceHashes=@($sourcePaths|ForEach-Object{[ordered]@{path=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash.ToLowerInvariant()}})
foreach($source in $sourceHashes){$destination=Join-Path $run ('sources/'+$source.path);New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null;Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $destination;if((Get-FileHash -LiteralPath $destination).Hash.ToLowerInvariant() -ne $source.sha256){throw 'Source snapshot changed'}}
$arguments=@([string]$ProcessId,('"{0}"' -f $exe),('"{0}"' -f (Join-Path $run 'dialog.png')),$mode)
$exitCode=$null;$launchError=$null;$timedOut=$false
try{
  $helperProcess=Start-Process -FilePath $helper -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'dialog.jsonl') -RedirectStandardError (Join-Path $run 'stderr.txt')
  $timedOut=-not $helperProcess.WaitForExit(10000)
  if($timedOut){Stop-Process -Id $helperProcess.Id;$helperProcess.WaitForExit()}
  $helperProcess.Refresh();$exitCode=$helperProcess.ExitCode
}catch{$launchError=$_.Exception.Message}
[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');pid=$ProcessId;mode=$mode;exe=$exe;exeSha256=$expected;helperSha256=(Get-FileHash -LiteralPath $helper).Hash.ToLowerInvariant();exitCode=$exitCode;launchError=$launchError;timedOut=$timedOut;sources=$sourceHashes;sourceSnapshot='sources';scope='Known Producer startup dialog only; no registry or security changes'} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if(Test-Path -LiteralPath (Join-Path $run 'dialog.jsonl')){Get-Content -LiteralPath (Join-Path $run 'dialog.jsonl')}
if($launchError -or $timedOut -or $exitCode -ne 0){throw 'Startup dialog helper failed; inspect retained evidence'}

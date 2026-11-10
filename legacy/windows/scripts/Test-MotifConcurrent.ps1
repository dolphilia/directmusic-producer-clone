[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Style,[string]$MotifName='Authored Motif')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summary=[IO.Path]::GetFullPath($BuildSummaryPath);$build=Get-Content -LiteralPath $summary -Raw|ConvertFrom-Json
if(-not $build.passed){throw 'Successful build required'}
foreach($s in $build.sources){if((Hash (Join-Path $repo $s.path)) -ne $s.sha256 -or (Hash (Join-Path $build.sourceRoot $s.path)) -ne $s.sha256){throw 'Source identity mismatch'}}
$identity=@($build.outputs|Where-Object path -eq 'install/bin/Producer.exe');$exe=Join-Path (Split-Path $summary -Parent) 'install/bin/Producer.exe'
if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Executable identity mismatch'}
$input=[IO.Path]::GetFullPath($Style);if($MotifName.Contains('"')){throw 'Unsupported name'}
$run=Join-Path $repo ('work/acceptance/motif-concurrent/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$args=@('--motif-concurrent-observe',('"'+(Join-Path $run 'concurrent')+'"'),('"'+$input+'"'),('"'+$MotifName+'"'))
$p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'stdout.txt') -RedirectStandardError (Join-Path $run 'stderr.txt')
$terminal=$p.WaitForExit(20000);$exit=$null;if($terminal){$p.Refresh();$exit=$p.ExitCode}
$passed=$terminal -and $exit -eq 0
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summary;buildSummarySha256=Hash $summary;input=$input;inputSha256=Hash $input;driverSha256=Hash $PSCommandPath;cases=@([ordered]@{name='motif-concurrent';executable=$exe;sha256=Hash $exe;arguments=$args;processId=$p.Id;exitCode=$exit;timedOut=(-not $terminal);passed=$passed});passed=$passed;scope='Concurrent Motif instance ownership and individual Stop API; GUI/audio separate';fullAcceptance=$false}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if(-not $terminal){throw ('Process remains live PID '+$p.Id+'; inspect same process before another run')}
if(-not $passed){throw 'Concurrent API failed; evidence retained'}

[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
function Output([string]$name){$item=@($summary.outputs|Where-Object path -eq $name);if($item.Count -ne 1){throw 'Output missing'};$p=Join-Path (Split-Path $summaryPath -Parent) $name;if((Hash $p) -ne $item[0].sha256){throw 'Output hash mismatch'};return $p}
$coreExe=Output 'build/Release/producer_core_tests.exe';$exe=Output 'install/bin/Producer.exe'
$dir=Join-Path $repo ('work/acceptance/runtime-recovery-inspect/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$core=Join-Path $dir 'core'
$p=Start-Process -FilePath $coreExe -ArgumentList @(('"'+$core+'"'),'--runtime-recovery-inspect') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir 'stdout.txt') -RedirectStandardError (Join-Path $dir 'stderr.txt')
$timeout=-not $p.WaitForExit(15000);$exit=$null;if(-not $timeout){$p.Refresh();$exit=$p.ExitCode}
$inputs=@();$cases=@();$failure=$null
if(-not $timeout -and $exit -eq 0){
    $inputs=@(Get-ChildItem -LiteralPath $core -File|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
    foreach($name in @('inspect','protected')){
        $report=Join-Path $dir ($name+'-report.json');$arguments=@('--inspect-runtime-recovery',('"'+(Join-Path $core 'core.pro')+'"'),('"'+(Join-Path $core ($name+'.riff'))+'"'),('"'+$report+'"'))
        $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
        $timedOut=-not $process.WaitForExit(15000);$code=$null;if(-not $timedOut){$process.Refresh();$code=$process.ExitCode}
        $cases+= [ordered]@{name=$name;processId=$process.Id;executable=$exe;sha256=Hash $exe;arguments=$arguments;exitCode=$code;timedOut=$timedOut;report=$report;passed=($code -eq 0 -and -not $timedOut)}
        if($timedOut -or $code -ne 0){$failure='Product recovery inspection failed';break}
    }
    foreach($i in $inputs){if((Hash $i.path) -ne $i.sha256){$failure='Read-only inspection changed an input'}}
}
$passed=-not $timeout -and $exit -eq 0 -and $null -eq $failure -and $cases.Count -eq 2
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$coreExe;exeSha256=Hash $coreExe;productExeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;processId=$p.Id;exitCode=$exit;timedOut=$timeout;inputs=$inputs;cases=$cases;error=$failure;passed=$passed;scope='Manifest parser/native conflict states and separate Producer.exe read-only Project-bound inspection; restore/crash/GUI/audio unexecuted';fullAcceptance=$false}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir)
if(-not $passed){throw 'Recovery inspection test failed; retain evidence'}

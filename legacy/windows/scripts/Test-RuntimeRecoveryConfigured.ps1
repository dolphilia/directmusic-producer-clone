[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string]$Project='work/producer/samples/Tutorial/FinishedProject/APFarm.aup')
$ErrorActionPreference='Stop'
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$repo=Split-Path $PSScriptRoot -Parent;$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Verified source build required'}
foreach($s in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Saved/current source mismatch'}}
function Output([string]$name){$item=@($summary.outputs|Where-Object path -eq $name);if($item.Count -ne 1){throw 'Output missing'};$p=Join-Path (Split-Path $summaryPath -Parent) $name;if((Hash $p) -ne $item[0].sha256){throw 'Output hash mismatch'};return $p}
$coreExe=Output 'build/Release/producer_core_tests.exe';$exe=Output 'install/bin/Producer.exe';$inputProject=[IO.Path]::GetFullPath($Project)
$dir=Join-Path $repo ('work/acceptance/runtime-recovery-configured/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $dir|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1');$core=Join-Path $dir 'core'
$cases=[Collections.Generic.List[object]]::new();$inputs=@([ordered]@{path=$inputProject;sha256=Hash $inputProject});$foreignProof=$null;$journal=$null;$failure=$null
function Run([string]$name,[string]$program,[string[]]$arguments,[int]$expected){
    $process=Start-Process -FilePath $program -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dir ($name+'-stdout.txt')) -RedirectStandardError (Join-Path $dir ($name+'-stderr.txt'))
    $timeout=-not $process.WaitForExit(15000);$exit=$null;if(-not $timeout){$process.Refresh();$exit=$process.ExitCode}
    $case=[ordered]@{name=$name;processId=$process.Id;executable=$program;sha256=Hash $program;arguments=$arguments;exitCode=$exit;expectedExit=$expected;timedOut=$timeout;passed=($exit -eq $expected -and -not $timeout)};$cases.Add($case)
    if(-not $case.passed){throw ('Failed '+$name+'; retain process/evidence without replay')}
}
try{
    Run 'prepare' $coreExe @(('"'+$core+'"'),'--runtime-recovery-configured',('"'+$inputProject+'"')) 0
    $journal=[Text.Encoding]::Unicode.GetString([IO.File]::ReadAllBytes((Join-Path $core 'recovery-location.txt'))).TrimEnd([char]0)
    $project=Join-Path $core 'Source/Source.pro';$runtime=Join-Path $core 'Output';$a=Join-Path $runtime 'A/Renamed.sgt';$b=Join-Path $runtime 'B/Renamed.sty'
    $inputs=@(Get-ChildItem -LiteralPath (Join-Path $core 'Source') -File -Recurse|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})+@([ordered]@{path=$journal;sha256=Hash $journal},[ordered]@{path=$inputProject;sha256=Hash $inputProject})
    Run 'before' $exe @('--inspect-runtime-recovery',('"'+$project+'"'),('"'+$journal+'"'),('"'+(Join-Path $dir 'before.json')+'"')) 0
    $publishedTargets=@(Get-ChildItem -LiteralPath $runtime -File -Recurse|Where-Object Name -ne 'retained.bin'|ForEach-Object {[ordered]@{path=$_.FullName;sha256=Hash $_.FullName}})
    $publishedB=[IO.File]::ReadAllBytes($b);$publishedA=Hash $a;[IO.File]::WriteAllBytes($b,[byte[]]@(0x55,0x66));$foreignB=Hash $b
    Run 'foreign-inspect' $exe @('--inspect-runtime-recovery',('"'+$project+'"'),('"'+$journal+'"'),('"'+(Join-Path $dir 'foreign-inspect.json')+'"')) 0
    Run 'foreign-recover' $exe @('--recover-runtime-update',('"'+$project+'"'),('"'+$journal+'"'),':defaults:',('"'+(Join-Path $dir 'foreign-recover.json')+'"')) 1
    $foreignProof=[ordered]@{aRetained=((Hash $a) -eq $publishedA);foreignBRetained=((Hash $b) -eq $foreignB);aSha256=Hash $a;bSha256=Hash $b;failedReportAbsent=(-not (Test-Path -LiteralPath (Join-Path $dir 'foreign-recover.json')))}
    $foreignProof.targets=@($publishedTargets|ForEach-Object {[ordered]@{path=$_.path;beforeSha256=$_.sha256;afterSha256=Hash $_.path;retained=((Hash $_.path) -eq $(if([IO.Path]::GetFullPath($_.path) -eq [IO.Path]::GetFullPath($b)){$foreignB}else{$_.sha256}))}})
    if(-not $foreignProof.aRetained -or -not $foreignProof.foreignBRetained -or @($foreignProof.targets|Where-Object {-not $_.retained}).Count){throw 'Foreign conflict altered a target'}
    # Undo only this driver's own sentinel in its freshly generated fixture.
    [IO.File]::WriteAllBytes($b,$publishedB)
    Run 'recover' $exe @('--recover-runtime-update',('"'+$project+'"'),('"'+$journal+'"'),':defaults:',('"'+(Join-Path $dir 'recover.json')+'"')) 0
    Run 'after' $exe @('--inspect-runtime-recovery',('"'+$project+'"'),('"'+$journal+'"'),('"'+(Join-Path $dir 'after.json')+'"')) 0
    Run 'repeat' $exe @('--recover-runtime-update',('"'+$project+'"'),('"'+$journal+'"'),':defaults:',('"'+(Join-Path $dir 'repeat.json')+'"')) 0
    Run 'verify' $coreExe @(('"'+$core+'"'),'--runtime-recovery-configured-verify') 0
    foreach($i in $inputs){if((Hash $i.path) -ne $i.sha256){throw 'Recovery changed a source or journal'}}
}catch{$failure=$_.Exception.Message}
$passed=$null -eq $failure -and $cases.Count -eq 8
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;coreExecutable=$coreExe;coreExeSha256=Hash $coreExe;productExecutable=$exe;productExeSha256=Hash $exe;driverSha256=Hash $PSCommandPath;journal=$journal;inputs=$inputs;cases=@($cases.ToArray());foreignConflict=$foreignProof;error=$failure;passed=$passed;scope='Actual retained configured five-form rollback, Project-bound separate-process recovery, foreign conflict rejection and idempotence; no forced process termination/OS bypass/crash/GUI/audio/full acceptance';fullAcceptance=$false}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir)
if(-not $passed){throw $failure}

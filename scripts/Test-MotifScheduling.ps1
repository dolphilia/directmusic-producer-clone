[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$Style,[string]$MotifName='Authored Motif')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed){throw 'Successful build required'}
foreach($s in $b.sources){if((Hash (Join-Path $repo $s.path)) -ne $s.sha256 -or (Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256){throw 'Saved/workspace identity mismatch'}}
$exe=Join-Path (Split-Path $summaryPath -Parent) 'install/bin/Producer.exe';$identity=@($b.outputs|Where-Object path -eq 'install/bin/Producer.exe');if($identity.Count -ne 1 -or (Hash $exe) -ne $identity[0].sha256){throw 'Executable identity mismatch'}
$input=[IO.Path]::GetFullPath($Style);if($MotifName.Contains('"')){throw 'Unsupported name'}
$run=Join-Path $repo ('work/acceptance/motif-scheduling/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null;Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1')
$results=@();$all=$true
foreach($case in @(@{name='scheduled-primary';delay=3072;boundary=0;prepare=0;secondary=0;flags=0},@{name='stored-secondary';delay=3072;boundary=1;prepare=1;secondary=1;flags=17536})){
  $dir=Join-Path $run $case.name;$args=@('--style-motif-scheduled-observe',('"'+$dir+'"'),('"'+$input+'"'),('"'+$MotifName+'"'),[string]$case.delay,[string]$case.boundary,[string]$case.prepare,[string]$case.secondary)
  $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run ($case.name+'.stdout.txt')) -RedirectStandardError (Join-Path $run ($case.name+'.stderr.txt'))
  $terminal=$p.WaitForExit(20000);if(-not $terminal){throw ('Scheduling process remains live PID '+$p.Id+'; inspect before another run')};$p.Refresh()
  $n=Get-Content -LiteralPath (Join-Path $dir 'notes.json') -Raw|ConvertFrom-Json;$r=Get-Content -LiteralPath (Join-Path $dir 'playback-request.json') -Raw|ConvertFrom-Json
  $passed=$p.ExitCode -eq 0 -and $n.passed -and $r.flags -eq $case.flags -and $r.delayClocks -eq $case.delay -and $r.requestedClocks -eq ($r.submittedClocks+$case.delay) -and $n.start -eq $r.requestedClocks -and $n.notes.Count -eq 6
  for($i=0;$i -lt $n.notes.Count;$i++){if($n.notes[$i].clocks -ne ($n.start+768*$i) -or $n.notes[$i].midiValue -ne 72 -or $n.notes[$i].channel -ne 5){$passed=$false}}
  if((Hash (Join-Path $dir 'source-style-0.stp')) -ne (Hash $input)){$passed=$false}
  $results+=[ordered]@{name=$case.name;pid=$p.Id;executable=$exe;exeSha256=Hash $exe;arguments=$args;exitCode=$p.ExitCode;request=$r;actualStart=$n.start;notesSha256=Hash (Join-Path $dir 'notes.json');passed=$passed};$all=$all -and $passed
}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;input=$input;inputSha256=Hash $input;driverSha256=Hash $PSCommandPath;cases=$results;passed=$all;scope='Separate primary and secondary-only scheduled Motif API runs; no simultaneous playback/GUI/audio acceptance';fullAcceptance=$false}|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run);if(-not $all){throw 'Scheduling verification failed'}

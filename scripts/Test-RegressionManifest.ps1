[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[string[]]$Only=@())
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){
  if(Test-Path -LiteralPath $p -PathType Container){
    $base=[IO.Path]::GetFullPath($p)
    $lines=@(Get-ChildItem -LiteralPath $base -File -Recurse|Sort-Object FullName|ForEach-Object {([IO.Path]::GetRelativePath($base,$_.FullName).Replace('\','/'))+':'+(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()})
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([Text.Encoding]::UTF8.GetBytes(($lines -join "`n")))).ToLowerInvariant()
  }
  return (Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()
}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath);$b=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $b.passed -or -not $b.sourceSnapshotUnchanged){throw 'Successful immutable build required'}
foreach($s in $b.sources){if((Hash (Join-Path $b.sourceRoot $s.path)) -ne $s.sha256 -or (Hash (Join-Path $repo $s.path)) -ne $s.sha256){throw 'Candidate source mismatch'}}
$manifestPath=Join-Path $repo 'docs/analysis/regression-manifest.json';$manifest=Get-Content -LiteralPath $manifestPath -Raw|ConvertFrom-Json
if((Hash (Join-Path $repo $manifest.sourceDispatcher.path)) -ne $manifest.sourceDispatcher.sha256){throw 'Dispatcher inventory stale'}
$o=@($b.outputs|Where-Object path -eq 'build/Release/producer_core_tests.exe');$exe=Join-Path (Split-Path $summaryPath -Parent) $o[0].path
if((Hash $exe) -ne $o[0].sha256){throw 'Candidate executable mismatch'}
$run=Join-Path $repo ('work/acceptance/regression/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'driver.ps1');Copy-Item -LiteralPath $manifestPath -Destination (Join-Path $run 'manifest.json')
$results=[Collections.Generic.List[object]]::new()
$refusedRecovery=$null
foreach($t in $manifest.tests){
  if($Only.Count -and $t.id -notin $Only){continue}
  $case=Join-Path $run $t.id;New-Item -ItemType Directory -Path $case|Out-Null
  $inputs=@($t.inputs|ForEach-Object {if(Test-Path -LiteralPath $_){[ordered]@{path=[IO.Path]::GetFullPath($_);sha256=Hash $_}}})
  $r=[ordered]@{id=$t.id;status='未実行';inputs=$inputs;arguments=@();processId=$null;exitCode=$null;timedOut=$false;checks=$null;error=$null;evidence=$case}
  if($t.blockedByKnownOsRefusal){$r.status='障害あり';$r.error=$t.blockedByKnownOsRefusal.reason}
  elseif($refusedRecovery -and $t.id -like 'runtime-recovery-*'){$r.status='障害あり';$r.error='Earlier fresh Recovery publication refused; no unchanged retry: '+$refusedRecovery}
  elseif($t.workflow){$r.status='検証待ち';$r.error=$t.workflow}
  elseif($inputs.Count -ne @($t.inputs).Count){$r.status='障害あり';$r.error='Required input missing'}
  else {
    $arguments=@(('"'+(Join-Path $case 'core')+'"'))+@($t.arguments)+@($inputs|ForEach-Object {'"'+$_.path+'"'});$r.arguments=$arguments
    try {
      $p=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $case 'stdout.txt') -RedirectStandardError (Join-Path $case 'stderr.txt');$r.processId=$p.Id
      $caseTimeout=if($t.timeoutMs){[int]$t.timeoutMs}else{15000};if($caseTimeout -lt 1000 -or $caseTimeout -gt 120000){throw 'Invalid registered timeout'}
      $r.timedOut=-not $p.WaitForExit($caseTimeout)
      if($r.timedOut){$r.status='障害あり';$r.error='Timeout; process retained; no unchanged retry'}
      else {$p.Refresh();$r.exitCode=$p.ExitCode;$r.status='失敗';if($r.exitCode -eq 0){$json=@(Get-Content (Join-Path $case 'stdout.txt')|Where-Object {$_ -match '^\{"(?:passed|diagnosticsPassed)":'}) ;if($json.Count -ne 1){throw 'Exactly one native result required'};$native=$json[0]|ConvertFrom-Json;$r.checks=$native.checks;if($native.passed){$r.status='合格'}elseif($t.id -eq 'script-sender-diagnostic' -and $native.diagnosticsPassed -and $native.traceAcceptance -eq $false){$r.status='合格';$r['scope']='Sender diagnostic and SDK invariance only; exact Trace acceptance false';$r['traceAcceptance']=$false}}}
      if($r.exitCode -ne 0 -and $t.id -like 'runtime-recovery-*'){
        $diagnostic=Get-Content (Join-Path $case 'stderr.txt') -Raw
        # These tests deliberately lock rollback outputs. Only a refusal in
        # the ORIGINAL publication cause blocks later preparations.
        if($diagnostic -match '(?m)recovery-prepare publications: .*?error: (?:Atomic save failed[^\r\n]*Windows error (?:5|1260)|Runtime update rollback incomplete; original failure: Atomic save failed[^\r\n]*Windows error (?:5|1260))'){$refusedRecovery=$case;$r.error='OS publication refusal; original cause retained in stderr'}
      }
      foreach($i in $inputs){if((Hash $i.path) -ne $i.sha256){$r.status='失敗';$r.error='Input changed'}}
    } catch {$r.status='障害あり';$r.error=$_.Exception.Message}
  }
  $r|ConvertTo-Json -Depth 8|Set-Content (Join-Path $case 'run.json') -Encoding UTF8;$results.Add($r)
  Write-Output ($t.id+': '+$r.status)
}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');candidate=Split-Path (Split-Path $summaryPath -Parent) -Leaf;buildSummary=$summaryPath;buildSummarySha256=Hash $summaryPath;executable=$exe;exeSha256=Hash $exe;manifestSha256=Hash $manifestPath;driverSha256=Hash $PSCommandPath;environment=[Environment]::OSVersion.VersionString;results=$results;drivers=$manifest.drivers;fullAcceptance=$false}|ConvertTo-Json -Depth 12|Set-Content (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)

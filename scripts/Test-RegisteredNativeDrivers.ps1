[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$manifest=Get-Content (Join-Path $repo 'docs/analysis/regression-manifest.json') -Raw|ConvertFrom-Json
$run=Join-Path $repo ('work/acceptance/registered-drivers/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));New-Item -ItemType Directory -Path $run|Out-Null
$results=[Collections.Generic.List[object]]::new()
foreach($d in $manifest.drivers){
  $r=[ordered]@{id=$d.id;runner=$d.runner;status='未実行';reason=$null;parameters=@{};evidence=$null;error=$null}
  if($d.blockedByKnownOsRefusal){$r.status='障害あり';$r.reason=$d.blockedByKnownOsRefusal.reason;$r.evidence=$d.blockedByKnownOsRefusal.evidence}
  elseif($d.runner -notlike '*.ps1'){$r.reason='Auditor needs a current producing run and a new controls directory'}
  else {
    $file=Join-Path $repo $d.runner;$tokens=$null;$errors=$null;$ast=[Management.Automation.Language.Parser]::ParseFile($file,[ref]$tokens,[ref]$errors)
    $params=$ast.ParamBlock.Parameters; $names=@($params|ForEach-Object {$_.Name.VariablePath.UserPath})
    $mode=@($manifest.tests|Where-Object {$_.arguments.Count -and $_.arguments[0] -in $d.dedicatedModes -and -not $_.workflow}|Select-Object -First 1)
    if('BuildSummaryPath' -notin $names){$r.reason='Needs preparation/GUI/current audio run rather than direct build input'}
    elseif(-not $mode.Count -and $d.id -notin @('Test-ProductHost','Test-ProductStartupFailures','Test-SourceProductDeployment')){$r.reason='Runtime/audio/GUI workflow requires explicit fresh preparation; not silently invoked with legacy inputs'}
    else {
      $args=@{BuildSummaryPath=$BuildSummaryPath};$unknown=@()
      foreach($p in $params){$name=$p.Name.VariablePath.UserPath
        if($name -eq 'BuildSummaryPath'){continue}
        $required=$p.Extent.Text -match 'Mandatory';if(-not $required){continue}
        if($name -eq 'Dls' -and $mode.Count -and $mode[0].inputs.Count -eq 1){$args[$name]=$mode[0].inputs[0]}
        elseif($name -eq 'Segment' -and $mode.Count -and $mode[0].inputs.Count -eq 2){$args[$name]=$mode[0].inputs[0]}
        elseif($name -eq 'Style' -and $mode.Count -and $mode[0].inputs.Count -eq 2){$args[$name]=$mode[0].inputs[1]}
        else {$unknown+=$name}
      }
      if($unknown.Count){$r.reason='Unresolved required input: '+($unknown -join ', ')}
      else {
        $r.parameters=$args;$log=Join-Path $run ($d.id+'.log');$r.evidence=$log
        try {& $file @args *> $log;$r.status='合格'}catch{$r.status='失敗';$r.error=$_.Exception.Message}
        Write-Output ($d.id+': '+$r.status)
      }
    }
  }
  $results.Add($r)
}
[ordered]@{schema=1;candidate=Split-Path (Split-Path $BuildSummaryPath -Parent) -Leaf;buildSummary=[IO.Path]::GetFullPath($BuildSummaryPath);buildSummarySha256=(Get-FileHash $BuildSummaryPath).Hash.ToLowerInvariant();createdUtc=[DateTime]::UtcNow.ToString('o');results=@($results.ToArray());scope='One driver inventory pass; unmet prerequisites remain unexecuted; native/GUI/audio/full acceptance separate';fullAcceptance=$false}|ConvertTo-Json -Depth 9|Set-Content (Join-Path $run 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)

[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunPath)
$ErrorActionPreference='Stop'
function Hash([string]$path){(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant()}
$runPathFull=[IO.Path]::GetFullPath($RunPath);$run=Get-Content -LiteralPath $runPathFull -Raw|ConvertFrom-Json
$directory=Split-Path $runPathFull -Parent;$base=Join-Path $directory 'style-playback'
$case=@($run.cases|Where-Object name -eq 'style-playback-api')
if($case.Count -ne 1 -or -not $case[0].passed -or (Hash $case[0].executable) -ne $case[0].sha256){throw 'Successful verified Style playback case required'}
if((Hash $run.buildSummary) -ne $run.buildSummarySha256){throw 'Build identity changed'}
$playbackPath=Join-Path $base 'playback.json';$playback=Get-Content -LiteralPath $playbackPath -Raw|ConvertFrom-Json
if(-not $playback.passed -or -not $playback.alteredMemoryMeter3_4 -or $playback.styleCount -ne 1){throw 'One-Style memory meter proof required'}
$input=Join-Path $base 'input.sgp';$style=Join-Path $base 'style-0.stp';$probe=Join-Path $base 'memory-meter-probe.stp'
if((Hash $input) -ne $run.referenceInput.sha256){throw 'Played document differs from recorded input'}
$sourceStyles=@($run.referenceStyles.files|Where-Object sha256 -eq (Hash $style));if($sourceStyles.Count -ne 1){throw 'Played Style copy lacks a unique recorded source identity'}
if((Hash $sourceStyles[0].path) -ne $sourceStyles[0].sha256){throw 'Original Style input changed during proof'}
$diffPath=Join-Path $base 'memory-meter-diff.json';$diff=Get-Content -LiteralPath $diffPath -Raw|ConvertFrom-Json
if($diff.left.sha256 -ne (Hash $style) -or $diff.right.sha256 -ne (Hash $probe) -or $diff.changes.Count -ne 1 -or $diff.changes[0].location -ne 'RIFF:DMST[0]/styh[0]'){throw 'Expected only Style header mutation'}
$old=[Convert]::FromHexString($diff.changes[0].left.hex);$changed=[Convert]::FromHexString($diff.changes[0].right.hex)
if($old[0] -ne 4 -or $old[1] -ne 4 -or $old[2] -ne 4 -or $changed[0] -ne 3 -or $changed[1] -ne 4 -or $changed[2] -ne 2){throw 'Meter mutation values differ from proof'}
$empty=@(Get-ChildItem -LiteralPath (Join-Path $base 'runtime-empty') -Force).Count -eq 0
if(-not $empty){throw 'Style search directory is no longer empty'}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');auditorSha256=(Hash $PSCommandPath);run=$runPathFull;runSha256=(Hash $runPathFull);buildSummarySha256=$run.buildSummarySha256;executableSha256=$case[0].sha256;
  playbackSha256=(Hash $playbackPath);documentSha256=(Hash $input);sourceStyle=$sourceStyles[0];ownedStyleSha256=(Hash $style);memoryProbeSha256=(Hash $probe);memoryProbeDiffSha256=(Hash $diffPath);
  sourceMeter='4/4 grids4';memoryOnlyProbeMeter='3/4 grids2';sameGuidAndNonHeaderChunks=$true;runtimeMemoryMeterMatched=$playback.alteredMemoryMeter3_4;searchDirectoryEmptyAtAudit=$empty;
  nativeApiPassed=$playback.passed;scope='One QuickStart Style reference; source copy and header-only memory mutation prove this tested meter used owned bytes. No general file-open trace, audible output, Pattern edit or full acceptance.'
}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $base 'identity-proof.json') -Encoding UTF8
Write-Output ('Evidence: '+(Join-Path $base 'identity-proof.json'))

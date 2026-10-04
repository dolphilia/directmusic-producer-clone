[CmdletBinding()]
param([Parameter(Mandatory)][string]$RunPath,[ValidateSet('host-smoke','playback-api','style-playback-api','dls-playback-api','group-playback-api','notifications-api','notes-api','audio-lifecycle','audio-tempo','audio-pattern','motif-concurrent','audio-concurrent','audio-primary-replacement','audio-primary-cancel','short-monitor','notification-identity','boundary-runtime')][string]$CaseName='host-smoke')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$runFile=[IO.Path]::GetFullPath($RunPath)
$run=Get-Content -LiteralPath $runFile -Raw | ConvertFrom-Json
$directory=Split-Path $runFile -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
if((Hash $run.buildSummary) -ne $run.buildSummarySha256){throw 'Build summary identity mismatch'}
$hostCase=@($run.cases | Where-Object name -eq $CaseName)
if($hostCase.Count -ne 1 -or -not $hostCase[0].passed){throw 'Successful host smoke required'}
$smokeFile=Join-Path $directory $(if($CaseName -eq 'host-smoke'){'host/smoke.json'}elseif($CaseName -eq 'style-playback-api'){'style-playback/playback.json'}elseif($CaseName -eq 'dls-playback-api'){'dls-playback/playback.json'}elseif($CaseName -eq 'group-playback-api'){'group-playback/playback.json'}elseif($CaseName -eq 'notifications-api'){'notifications/notifications.json'}elseif($CaseName -eq 'notes-api'){'notes/notes.json'}elseif($CaseName -eq 'audio-lifecycle'){'lifecycle/lifecycle.json'}elseif($CaseName -eq 'audio-tempo'){'tempo/notes.json'}elseif($CaseName -eq 'boundary-runtime'){'concurrent/boundaries.json'}elseif($CaseName -eq 'notification-identity'){'concurrent/identity-stress.json'}elseif($CaseName -eq 'short-monitor'){'short/short-monitor.json'}elseif($CaseName -eq 'audio-primary-cancel'){'concurrent/primary-cancel.json'}elseif($CaseName -eq 'audio-primary-replacement'){'concurrent/primary-replacement.json'}elseif($CaseName -eq 'audio-concurrent'){'concurrent/concurrent-audio.json'}elseif($CaseName -eq 'motif-concurrent'){'concurrent/concurrent.json'}elseif($CaseName -eq 'audio-pattern'){'crud/notes.json'}else{'playback/playback.json'})
$smoke=Get-Content -LiteralPath $smokeFile -Raw | ConvertFrom-Json
$original=Import-Csv -LiteralPath (Join-Path $repo 'docs/analysis/modules.csv')
$modules=@()
foreach($hex in $smoke.modulePathsUtf16Hex){
    if($hex -notmatch '^(?:[0-9a-f]{4})+$'){throw 'Invalid UTF-16 module path'}
    $bytes=New-Object byte[] ($hex.Length/2)
    for($i=0;$i -lt $bytes.Length;$i++){$bytes[$i]=[Convert]::ToByte($hex.Substring($i*2,2),16)}
    $reported=[Text.Encoding]::Unicode.GetString($bytes).TrimEnd([char]0)
    $physical=$reported;$basis='reported physical path'
    # The x86 inventory sees the WOW64 virtual System32 namespace. Hashing
    # System32 from this 64-bit auditor would identify the wrong DLL.
    if([Environment]::Is64BitOperatingSystem -and $reported.StartsWith((Join-Path $env:windir 'System32\'),[StringComparison]::OrdinalIgnoreCase)){
        $physical=Join-Path $env:windir ('SysWOW64\'+[IO.Path]::GetFileName($reported));$basis='x86 process: WOW64 System32 resolves to SysWOW64'
    }
    $sha=Hash $physical;$image=[IO.File]::ReadAllBytes($physical)
    if($image.Length -lt 64){throw 'Truncated module'}
    $pe=[BitConverter]::ToInt32($image,60)
    if($pe -lt 0 -or $pe+6 -gt $image.Length -or [BitConverter]::ToUInt32($image,$pe) -ne 0x4550){throw 'Invalid PE module'}
    $machine=[BitConverter]::ToUInt16($image,$pe+4)
    $matches=@($original | Where-Object sha256 -eq $sha | ForEach-Object input)
    $origin='unresolved'
    if($reported -eq $hostCase[0].executable -and $sha -eq $hostCase[0].sha256){$origin='this verified source snapshot'}
    elseif($physical.StartsWith((Join-Path $env:windir 'SysWOW64\'),[StringComparison]::OrdinalIgnoreCase) -or $physical.StartsWith((Join-Path $env:windir 'WinSxS\x86_'),[StringComparison]::OrdinalIgnoreCase)){$origin='installed Windows x86 system component'}
    $version=[Diagnostics.FileVersionInfo]::GetVersionInfo($physical)
    $modules += [ordered]@{reportedPath=$reported;physicalPath=$physical;resolutionBasis=$basis;sha256=$sha;machine=('0x{0:x4}' -f $machine);fileVersion=$version.FileVersion;company=$version.CompanyName;origin=$origin;originalHashMatches=$matches}
}
$passed=@($modules | Where-Object {$_.origin -eq 'unresolved' -or $_.machine -ne '0x014c' -or $_.originalHashMatches.Count -gt 0}).Count -eq 0
$output=Join-Path $directory $(if($CaseName -eq 'host-smoke'){'module-provenance.json'}elseif($CaseName -eq 'style-playback-api'){'style-playback-module-provenance.json'}elseif($CaseName -eq 'dls-playback-api'){'dls-playback-module-provenance.json'}elseif($CaseName -eq 'group-playback-api'){'group-playback-module-provenance.json'}elseif($CaseName -eq 'notifications-api'){'notification-module-provenance.json'}elseif($CaseName -eq 'notes-api'){'notes-module-provenance.json'}elseif($CaseName -eq 'audio-lifecycle'){'lifecycle-module-provenance.json'}elseif($CaseName -eq 'audio-tempo'){'tempo-module-provenance.json'}elseif($CaseName -eq 'boundary-runtime'){'boundary-module-provenance.json'}elseif($CaseName -eq 'notification-identity'){'identity-module-provenance.json'}elseif($CaseName -eq 'short-monitor'){'short-monitor-module-provenance.json'}elseif($CaseName -eq 'audio-primary-cancel'){'primary-cancel-module-provenance.json'}elseif($CaseName -eq 'audio-primary-replacement'){'primary-replacement-module-provenance.json'}elseif($CaseName -eq 'audio-concurrent'){'concurrent-audio-module-provenance.json'}elseif($CaseName -eq 'motif-concurrent'){'concurrent-module-provenance.json'}elseif($CaseName -eq 'audio-pattern'){'crud-module-provenance.json'}else{'playback-module-provenance.json'})
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');run=$runFile;runSha256=(Hash $runFile);smokeSha256=(Hash $smokeFile);auditorSha256=(Hash $PSCommandPath);originalInventorySha256=(Hash (Join-Path $repo 'docs/analysis/modules.csv'));scope=($CaseName+' captured module paths only; GUI and other paths not covered');moduleCount=$modules.Count;originalInventoryCount=$original.Count;passed=$passed;modules=$modules} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding UTF8
Write-Output ('Evidence: '+$output)
if(-not $passed){throw 'Unresolved, non-x86 or original-identity module; inspect evidence'}




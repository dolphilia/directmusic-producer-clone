[CmdletBinding()]
param([Parameter(Mandatory)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$dir=[IO.Path]::GetFullPath($EvidenceDirectory)
$state=Get-Content -LiteralPath (Join-Path $dir 'states.json') -Raw | ConvertFrom-Json
$capture=Get-Content -LiteralPath (Join-Path $dir 'gui-modules.json') -Raw | ConvertFrom-Json
$run=Get-Content -LiteralPath $state.run -Raw | ConvertFrom-Json
$hostCase=@($run.cases | Where-Object name -eq 'host-smoke')[0]
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
if($capture.error -or [IO.Path]::GetFullPath($capture.executable) -ne [IO.Path]::GetFullPath($state.executable) -or (Hash $capture.executable) -ne $hostCase.sha256){throw 'GUI capture identity or access failure'}
$build=Get-Content -LiteralPath $state.build -Raw | ConvertFrom-Json
$verifiedGuiOutputs=@($build.outputs | Where-Object {$_.sha256 -eq $hostCase.sha256 -and [IO.Path]::GetFullPath((Join-Path (Split-Path $state.build -Parent) $_.path)) -eq [IO.Path]::GetFullPath($capture.executable)})
if($verifiedGuiOutputs.Count -ne 1){throw 'GUI executable is not an output of the recorded source snapshot'}
$original=Import-Csv -LiteralPath (Join-Path $repo 'docs/analysis/modules.csv')
$modules=@();$entryIndex=0
foreach($reported in $capture.paths){
    $physical=$reported
    $ambiguous=$false
    $entry=if($capture.schema -eq 2){$capture.entries[$entryIndex]}else{$null};$entryIndex++
    $host64=$false
    if($entry){if($entry.reportedPath -ne $reported -or $entry.baseAddressHex -notmatch '^[0-9a-f]{16}$'){throw 'Invalid address capture'};$host64=[Convert]::ToUInt64($entry.baseAddressHex,16) -gt [uint32]::MaxValue}
    if($reported.StartsWith((Join-Path $env:windir 'System32\'),[StringComparison]::OrdinalIgnoreCase)){
        $candidate=Join-Path $env:windir ('SysWOW64\'+[IO.Path]::GetFileName($reported))
        if(-not $host64 -and (Test-Path -LiteralPath $candidate)){$physical=$candidate}
        # A 64-bit Get-Process inventory includes both ntdll images under the
        # same reported namespace. The capture omitted base addresses, so do
        # not infer which entry is the WOW64 host image.
        if(-not $entry -and [IO.Path]::GetFileName($reported) -ieq 'ntdll.dll'){$ambiguous=$true}
    }
    $sha=Hash $physical;$image=[IO.File]::ReadAllBytes($physical)
    $pe=[BitConverter]::ToInt32($image,60)
    if($pe -lt 0 -or $pe+6 -gt $image.Length -or [BitConverter]::ToUInt32($image,$pe) -ne 0x4550){throw 'Invalid PE module'}
    $machine=[BitConverter]::ToUInt16($image,$pe+4)
    $originalMatches=@($original | Where-Object sha256 -eq $sha | ForEach-Object input)
    $origin='unresolved'
    if($reported -eq $capture.executable -and $sha -eq $hostCase.sha256){$origin='this verified source snapshot'}
    elseif($physical.StartsWith((Join-Path $env:windir 'SysWOW64\'),[StringComparison]::OrdinalIgnoreCase) -or $physical.StartsWith((Join-Path $env:windir 'WinSxS\x86_'),[StringComparison]::OrdinalIgnoreCase)){$origin='installed Windows x86 system component'}
    $version=[Diagnostics.FileVersionInfo]::GetVersionInfo($physical)
    $signature=$null
    if($host64 -and $machine -eq 0x8664 -and $physical.StartsWith((Join-Path $env:windir 'System32\'),[StringComparison]::OrdinalIgnoreCase)){$origin='installed Windows WOW64 host support'}
    if($origin -eq 'unresolved'){
        $signed=Get-AuthenticodeSignature -LiteralPath $physical
        $signature=[ordered]@{status=[string]$signed.Status;subject=$signed.SignerCertificate.Subject;thumbprint=$signed.SignerCertificate.Thumbprint}
        if($signed.Status -eq 'Valid' -and $signed.SignerCertificate.Subject -match 'O=Microsoft Corporation' -and ($physical -match '\\Common Files\\microsoft shared\\ink\\' -or $physical -match '\\Microsoft\\OneDrive\\[^\\]+\\i386\\FileSyncShell.dll$')){$origin='signed Microsoft ambient input or shell integration'}
    }
    if($physical -eq $reported -and [IO.Path]::GetFileName($reported) -like 'wow64*.dll'){$origin='installed Windows WOW64 host support'}
    $modules += [ordered]@{reportedPath=$reported;physicalPath=$physical;baseAddressHex=$entry.baseAddressHex;resolutionAmbiguous=$ambiguous;sha256=$sha;machine=('0x{0:x4}' -f $machine);company=$version.CompanyName;fileVersion=$version.FileVersion;origin=$origin;signature=$signature;originalHashMatches=$originalMatches}
}
$passed=$modules.Count -gt 0 -and @($modules | Where-Object {$_.origin -eq 'unresolved' -or $_.resolutionAmbiguous -or ($_.machine -ne '0x014c' -and $_.origin -ne 'installed Windows WOW64 host support') -or $_.originalHashMatches.Count -gt 0}).Count -eq 0
$output=Join-Path $dir 'gui-module-provenance.json'
[ordered]@{schema=1;passed=$passed;createdUtc=[DateTime]::UtcNow.ToString('o');captureSha256=(Hash (Join-Path $dir 'gui-modules.json'));exeSha256=$hostCase.sha256;moduleCount=$modules.Count;originalInventoryCount=$original.Count;auditorSha256=(Hash $PSCommandPath);scope='One GUI process snapshot; capture time and address metadata in gui-modules.json, lifecycle in states.json; not a continuous inventory';modules=$modules} | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $output -Encoding UTF8
Write-Output ('Evidence: '+$output)
if(-not $passed){throw 'Unresolved, non-x86 or original-identity GUI module; inspect evidence'}

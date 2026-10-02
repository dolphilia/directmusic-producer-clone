[CmdletBinding()]
param(
    [string]$SevenZip = '7z.exe',
    [string]$Unshield,
    [switch]$VerifySecondMirror
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$downloads = Join-Path $repo 'artifacts/downloads'
$extracted = Join-Path $repo 'artifacts/extracted'
$expectedHash = '087B43E9EFC43915FDBD3352C1144A69A7F3A728E088B187F823B6B293839D6F'
$urls = @(
    'https://w2krepo.somnolescent.net/Audio/dx90_directmusicproducer.exe',
    'https://files.rajko.info/dx90_directmusicproducer.exe'
)
New-Item -ItemType Directory -Force -Path $downloads, $extracted | Out-Null
$null = Get-Command $SevenZip -ErrorAction Stop

function Assert-Package([string]$Path) {
    if ((Get-Item -LiteralPath $Path).Length -ne 33139712 -or
        (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $expectedHash) {
        throw "Unexpected package size/hash: $Path"
    }
}

function Get-Package([string]$Url, [string]$Destination) {
    if (Test-Path -LiteralPath $Destination) {
        Assert-Package $Destination
        return
    }
    $partial = "$Destination.partial"
    & curl.exe -L --fail --retry 2 --connect-timeout 20 --max-time 180 --silent --show-error -o $partial $Url
    if ($LASTEXITCODE -ne 0) { throw "Download failed: $Url" }
    Assert-Package $partial
    Move-Item -LiteralPath $partial -Destination $Destination
}

$package = Join-Path $downloads 'dx90_directmusicproducer.exe'
if (Test-Path -LiteralPath $package) {
    Assert-Package $package
} else {
    try { Get-Package $urls[0] $package }
    catch {
        Write-Warning $_
        Get-Package $urls[1] $package
    }
}
if ($VerifySecondMirror) {
    Get-Package $urls[1] (Join-Path $downloads 'dx90_directmusicproducer.rajko.exe')
}

# Treat both EXEs as archives; never execute the historical installers.
$wrapper = Join-Path $extracted 'dx90-wrapper'
$distribution = Join-Path $extracted 'dx90'
& $SevenZip x $package "-o$wrapper" -y -bso0
if ($LASTEXITCODE -ne 0) { throw 'Outer archive extraction failed.' }
& $SevenZip x (Join-Path $wrapper 'Essentials.exe') "-o$distribution" -y -bso0
if ($LASTEXITCODE -ne 0) { throw 'Essentials archive extraction failed.' }

if ($Unshield) {
    $null = Get-Command $Unshield -ErrorAction Stop
    $payload = Join-Path $extracted 'producer'
    $cabinet = Join-Path $distribution 'Essentials/DirectMusic Producer/data1.cab'
    & $Unshield -d $payload x $cabinet
    if ($LASTEXITCODE -ne 0) { throw 'InstallShield payload extraction failed.' }
}
Write-Output "Verified distribution: $distribution"
Write-Output "SHA256: $expectedHash"

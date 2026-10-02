[CmdletBinding()]
param([string]$SevenZip = '7z.exe')
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
Push-Location $repo
try {
    & node.exe scripts/Collect-PEEvidence.mjs
    if ($LASTEXITCODE -ne 0) { throw 'PE collection failed.' }
    & node.exe scripts/Inspect-Components.mjs
    if ($LASTEXITCODE -ne 0) { throw 'Component inspection failed.' }
    & $SevenZip x work/producer/app/dmusprod.chm '-owork/analysis/help' -y -bso0
    if ($LASTEXITCODE -ne 0) { throw 'Help extraction failed.' }
    foreach ($module in @('DMUSProd.exe', 'TempoStripMgr.dll', 'SegmentDesigner.ocx', 'Timeline.dll')) {
        & llvm-objdump.exe -d --x86-asm-syntax=intel --no-show-raw-insn "work/producer/app/$module" |
            Set-Content -LiteralPath "work/analysis/pe/app__$module/disassembly.txt" -Encoding UTF8
        if ($LASTEXITCODE -ne 0) { throw "Disassembly failed: $module" }
    }
} finally { Pop-Location }

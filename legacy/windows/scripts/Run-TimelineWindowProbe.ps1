[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$dll = Join-Path $repo 'work/producer/app/Timeline.dll'
$expected = 'bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176'
if ((Get-FileHash -LiteralPath $dll).Hash -ne $expected) { throw 'Reference Timeline DLL hash mismatch.' }
$exe = Join-Path $repo 'work/build/probes/Release/timeline_window_probe.exe'
$run = Join-Path $repo ('work/reference/timeline-window/' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run | Out-Null
$sources = @('CMakeLists.txt', 'scripts/Run-TimelineWindowProbe.ps1')
$sources += @(Get-ChildItem -LiteralPath (Join-Path $repo 'src'), (Join-Path $repo 'tests/native') -File -Recurse |
    ForEach-Object { $_.FullName.Substring($repo.Length + 1).Replace('\', '/') })
$sourceHashes = @($sources | ForEach-Object {
    $file = Join-Path $repo $_
    $target = Join-Path (Join-Path $run 'sources') $_
    New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $file -Destination $target
    $hash = (Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()
    if ((Get-FileHash -LiteralPath $target).Hash -ne $hash) { throw 'Source changed while taking snapshot.' }
    [ordered]@{ path = $_; sha256 = $hash }
})
$probeHash = (Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
$exitCode = $null
$launchError = $null
$timedOut = $false
try {
    $process = Start-Process -FilePath $exe -ArgumentList ('"{0}"' -f $dll) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $run 'probe.jsonl') -RedirectStandardError (Join-Path $run 'stderr.txt')
    $timedOut = -not $process.WaitForExit(15000)
    if ($timedOut) { Stop-Process -Id $process.Id; $process.WaitForExit() }
    $process.Refresh()
    $exitCode = $process.ExitCode
} catch { $launchError = $_.Exception.Message }
[ordered]@{
    createdUtc = [DateTime]::UtcNow.ToString('o'); os = [Environment]::OSVersion.VersionString
    dllSha256 = $expected; probeSha256 = $probeHash
    exitCode = $exitCode; launchError = $launchError; timedOut = $timedOut
    sourceHashes = $sourceHashes
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
Write-Output $run
if ($launchError) { throw $launchError }
if ($timedOut -or $exitCode -ne 0) { throw "Timeline window probe failed: exit=$exitCode timeout=$timedOut" }

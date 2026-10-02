[CmdletBinding()]
param([Alias('SaveEmptyStream')][switch]$SaveInitialStream, [switch]$Connected, [switch]$Timeline, [switch]$Candidate, [switch]$TimeSignature, [switch]$Windowed, [switch]$Clipboard,
    [string]$BuildDirectory = 'work/build/probes')
$ErrorActionPreference = 'Stop'
if ($Clipboard -and -not $Windowed) { throw 'Clipboard cases require the windowed Timeline fixture' }
if ($Windowed) { $TimeSignature = $true }
if ($TimeSignature) { $Timeline = $true }
if ($Timeline) { $Connected = $true }
if ($Connected) { $SaveInitialStream = $true }
$repo = Split-Path $PSScriptRoot -Parent
$probeBuild = if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $repo $BuildDirectory }
$dll = Join-Path $repo 'work/producer/app/TempoStripMgr.dll'
$expected = 'bb9811c74f68dcf0b37d32fe2ae89d3e45962e59b95f1ec93ddf7a12635a5c95'
if ($Candidate) {
    $dll = Join-Path $probeBuild 'Release/TempoStripMgr.dll'
    $expected = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash.ToLowerInvariant()
}
if ((Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash -ne $expected) { throw 'Reference DLL hash mismatch.' }
if ($Timeline -and (Get-FileHash -LiteralPath (Join-Path $repo 'work/producer/app/Timeline.dll')).Hash -ne 'bebc8149e31f3b4b0b74bffbf482acc38ba8b5b28a1468c9741e71961c6db176') {
    throw 'Reference Timeline DLL hash mismatch.'
}
if ($TimeSignature -and (Get-FileHash -LiteralPath (Join-Path $repo 'work/producer/app/TimeSigStripMgr.dll')).Hash -ne '898258cfbf1b17bef0054695d25331c530e2ee2846c5d45073a8fd5670484bb7') {
    throw 'Reference TimeSigStripMgr DLL hash mismatch.'
}
$exe = Join-Path $probeBuild 'Release/com_probe.exe'
if ($Windowed) { $exe = Join-Path $probeBuild 'Release/com_window_probe.exe' }
$runRoot = if ($Candidate) { 'work/candidate/tempo/' } else { 'work/reference/tempo/' }
$run = Join-Path $repo ($runRoot + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run | Out-Null
$probeArguments = @(('"{0}"' -f $dll), '"{C6ED2EA5-F1D3-11D1-88CB-00C04FBF8D15}"')
if ($SaveInitialStream) { $probeArguments += ('"{0}"' -f (Join-Path $run 'initial-stream.bin')) }
if ($Connected) { $probeArguments += '--connected' }
if ($Timeline) {
    $probeArguments += '--timeline'
    $probeArguments += ('"{0}"' -f (Join-Path $repo 'work/producer/app/Timeline.dll'))
}
if ($TimeSignature) { $probeArguments += ('"{0}"' -f (Join-Path $repo 'work/producer/app/TimeSigStripMgr.dll')) }
if ($Clipboard) { $probeArguments += '--clipboard' }
$sourcePaths = @('CMakeLists.txt', 'tests/native/producer_startup_probe.cpp', 'tests/native/com_probe.cpp', 'tests/native/drawing_surface.h', 'tests/native/tempo_host_fixture.h',
    'tests/native/timeline_window_probe.cpp', 'tests/native/hidden_ole_site.h', 'tests/native/drag_probe.h', 'tests/native/ole_drag_boundary.h', 'tests/native/drop_menu_boundary.h', 'tests/native/command_probe.h',
    'tests/native/runtime_tempo.h', 'tests/native/reference_timeline.h', 'tests/native/reference_tempo_edit.h', 'tests/native/clipboard_snapshot.h', 'src/compat/strip_manager.h',
    'src/compat/producer_ids.h', 'scripts/Run-ReferenceProbe.ps1', 'tests/native/reference_time_signature.h',
    'tests/native/tempo_core_compare.cpp', 'scripts/Compare-TempoCore.mjs', 'scripts/Compare-TempoDll.mjs')
# Keep all CMake inputs for both runs so that each source snapshot can configure
# the project independently, including targets other than com_probe.
$sourcePaths += @('src/tempo/tempo_track.h', 'src/tempo/tempo_track.cpp', 'src/tempo/tempo_dll.cpp',
        'src/compat/prop_page_manager.h', 'tests/native/property_page_probe.h',
        'src/tempo/tempo_pages.rc', 'src/tempo/tempo_data_object.h',
        'src/tempo/tempo_dll.def', 'src/compat/tempo_runtime.h', 'src/compat/prop_page_object.h', 'src/compat/host_services.h',
        'src/compat/strip.h', 'src/compat/timeline_services.h', 'src/compat/tempo_notifications.h')
$sourceHashes = @($sourcePaths | ForEach-Object {
    [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $repo $_) -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$snapshotRoot = Join-Path $run 'sources'
foreach ($source in $sourceHashes) {
    $snapshotFile = Join-Path $snapshotRoot $source.path
    New-Item -ItemType Directory -Path (Split-Path $snapshotFile -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $repo $source.path) -Destination $snapshotFile
    if ((Get-FileHash -LiteralPath $snapshotFile).Hash -ne $source.sha256) { throw 'Source changed while creating run snapshot.' }
}
$process = $null
$timedOut = $false
$launchError = $null
$exitCode = $null
try {
    $process = Start-Process -FilePath $exe -ArgumentList $probeArguments -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $run 'probe.jsonl') -RedirectStandardError (Join-Path $run 'stderr.txt')
    $timedOut = -not $process.WaitForExit(15000)
    if ($timedOut) { Stop-Process -Id $process.Id; $process.WaitForExit() }
    $process.Refresh()
    $exitCode = $process.ExitCode
} catch {
    $launchError = $_.Exception.Message
}
$runtimeModules = @()
$timelineModule = $null
$timeSignatureModule = $null
if ($TimeSignature) {
    $meterFile = Get-Item -LiteralPath (Join-Path $repo 'work/producer/app/TimeSigStripMgr.dll')
    $timeSignatureModule = [ordered]@{ path = $meterFile.FullName; version = $meterFile.VersionInfo.FileVersion
        sha256 = (Get-FileHash -LiteralPath $meterFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
}
if ($Timeline) {
    $timelineFile = Get-Item -LiteralPath (Join-Path $repo 'work/producer/app/Timeline.dll')
    $timelineModule = [ordered]@{ path = $timelineFile.FullName; version = $timelineFile.VersionInfo.FileVersion
        sha256 = (Get-FileHash -LiteralPath $timelineFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$logPath = Join-Path $run 'probe.jsonl'
if (Test-Path -LiteralPath $logPath) {
    $runtimeModules = @(Get-Content -LiteralPath $logPath | ForEach-Object { $_ | ConvertFrom-Json } |
        Where-Object operation -eq 'runtime_module' | ForEach-Object {
            $reportedPath = $_.path
            $resolvedPath = $reportedPath
            $resolution = 'reported'
            # The probe is x86. Its System32 path can denote the redirected
            # SysWOW64 file; this runner is normally a 64-bit PowerShell process.
            $system32Prefix = (Join-Path $env:windir 'System32') + '\'
            if ([Environment]::Is64BitProcess -and $reportedPath.StartsWith($system32Prefix, [StringComparison]::OrdinalIgnoreCase)) {
                $resolvedPath = Join-Path (Join-Path $env:windir 'SysWOW64') $reportedPath.Substring($system32Prefix.Length)
                $resolution = 'x86_System32_to_SysWOW64'
            }
            $runtimeFile = Get-Item -LiteralPath $resolvedPath
            [ordered]@{ reportedPath = $reportedPath; path = $runtimeFile.FullName; resolution = $resolution; version = $runtimeFile.VersionInfo.FileVersion
                sha256 = (Get-FileHash -LiteralPath $runtimeFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
        })
}
[ordered]@{
    createdUtc = [DateTime]::UtcNow.ToString('o'); os = [Environment]::OSVersion.VersionString
    dllSha256 = $expected; probeSha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
    saveInitialStream = [bool]$SaveInitialStream; exitCode = $exitCode; timedOut = $timedOut
    connectedFixture = [bool]$Connected
    originalTimeline = [bool]$Timeline
    windowedTimeline = [bool]$Windowed
    systemClipboard = [bool]$Clipboard
    buildDirectory = [IO.Path]::GetFullPath($probeBuild)
    timelineModule = $timelineModule
    timeSignatureModule = $timeSignatureModule
    implementation = $(if ($Candidate) { 'candidate' } else { 'original' })
    launchError = $launchError; sources = $sourceHashes; sourceSnapshot = 'sources'; runtimeModules = $runtimeModules
    registryRegistrationInvoked = $false
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $run 'run.json') -Encoding UTF8
if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath }
Write-Output "Evidence: $run"
if ($launchError) { throw "Reference probe could not run: $launchError" }
if ($timedOut -or $exitCode -ne 0) { throw 'Reference probe failed; see retained evidence.' }

[CmdletBinding()]
param([string]$CMake='C:/Program Files/CMake/bin/cmake.exe')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$run=Join-Path $repo ('work/build/time-signature-snapshot/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
$sourceRoot=Join-Path $run 'sources'
$build=Join-Path $run 'build'
New-Item -ItemType Directory -Path $sourceRoot|Out-Null
$sourcePaths=@('tests/native/time_signature_probe.cpp','tests/native/time_signature_connection_probe.cpp','tests/native/time_signature_style_probe.cpp','tests/native/reference_timeline.h','tests/native/time_signature/CMakeLists.txt',
    'src/compat/producer_ids.h','src/compat/strip_manager.h','src/compat/time_signature.h',
    'src/time_signature/CMakeLists.txt','src/time_signature/time_signature_map.h','src/time_signature/time_signature_map.cpp',
    'src/time_signature/time_signature_dll.cpp','src/time_signature/time_signature_dll.def',
    'src/compat/strip.h','src/compat/prop_page_object.h','src/compat/timeline_services.h',
    'scripts/Run-TimeSignatureProbe.ps1','scripts/Inspect-TimeSignatureAbi.mjs','scripts/Build-TimeSignatureSnapshot.ps1')
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$sources=@($sourcePaths|ForEach-Object {
    $source=Join-Path $repo $_
    $hash=Hash $source
    $destination=Join-Path $sourceRoot $_
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force|Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    if((Hash $destination) -ne $hash){throw 'Snapshot copy changed'}
    [ordered]@{path=$_;sha256=$hash}
})
$configureExit=$null;$buildExit=$null;$errorText=$null
try{
    & $CMake -S (Join-Path $sourceRoot 'tests/native/time_signature') -B $build -G 'Visual Studio 17 2022' -A Win32 *> (Join-Path $run 'configure.log')
    $configureExit=$LASTEXITCODE
    if($configureExit -ne 0){throw 'Snapshot configuration failed'}
    & $CMake --build $build --config Release *> (Join-Path $run 'build.log')
    $buildExit=$LASTEXITCODE
    if($buildExit -ne 0){throw 'Snapshot build failed'}
}catch{$errorText=$_.Exception.Message}
$outputs=@()
if(-not $errorText){
    $outputs=@('build/Release/time_signature_probe.exe','build/Release/time_signature_connection_probe.exe','build/Release/time_signature_style_probe.exe','build/Release/TimeSigStripMgr.dll','build/time_signature/Release/time_signature_core.lib')|ForEach-Object {
        $file=Join-Path $run $_
        [ordered]@{path=$_;bytes=(Get-Item -LiteralPath $file).Length;sha256=(Hash $file)}
    }
}
$sourceUnchanged=@($sources|Where-Object {(Hash (Join-Path $sourceRoot $_.path)) -ne $_.sha256}).Count -eq 0
$record=[ordered]@{createdUtc=[DateTime]::UtcNow.ToString('o');sourceRoot=$sourceRoot;buildDirectory=$build;sources=$sources;
    sourceSnapshotUnchanged=$sourceUnchanged;cmake=$CMake;cmakeSha256=(Hash $CMake);generator='Visual Studio 17 2022';architecture='Win32';configuration='Release';
    configureExitCode=$configureExit;buildExitCode=$buildExit;error=$errorText;outputs=$outputs;passed=(-not $errorText -and $sourceUnchanged);
    runtimeExecuted=$false;scope='Source snapshot configure and compile only. No execution, DLL loading, registration or host replacement.'}
$record|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $run 'build-summary.json') -Encoding utf8
Write-Output ('Evidence: '+$run)
if(-not $record.passed){throw 'Snapshot build failed; inspect preserved logs'}
Write-Output ('Built from '+$sources.Count+' saved source files; runtime acceptance remains unverified')

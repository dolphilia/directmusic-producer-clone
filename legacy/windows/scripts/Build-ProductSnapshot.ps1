[CmdletBinding()]
param([string]$CMake='C:/Program Files/CMake/bin/cmake.exe')
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$repoPrefix=$repo.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
function Get-SourceRelativePath([string]$Path) {
  $full=[IO.Path]::GetFullPath($Path)
  if(-not $full.StartsWith($repoPrefix,[StringComparison]::OrdinalIgnoreCase)){throw 'Source path escapes repository'}
  return $full.Substring($repoPrefix.Length).Replace('\','/')
}
$run=Join-Path $repo ('work/build/product-snapshot/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
$sourceRoot=Join-Path $run 'sources';$build=Join-Path $run 'build';$install=Join-Path $run 'install'
New-Item -ItemType Directory -Path $sourceRoot|Out-Null
Write-Output ('Source snapshot: '+$run)
$paths=@('CMakeLists.txt','CMakePresets.json','scripts/Build-ProductSnapshot.ps1','scripts/Test-ProductSnapshot.ps1','scripts/SourceProductDeployment.ps1','scripts/Install-SourceProduct.ps1','scripts/Uninstall-SourceProduct.ps1','docs/source-product-deployment.md')
foreach($directory in @('src','tests/producer')){
  $paths+=@(Get-ChildItem -LiteralPath (Join-Path $repo $directory) -File -Recurse|Sort-Object FullName|ForEach-Object {Get-SourceRelativePath $_.FullName})
}
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$sources=@($paths|ForEach-Object {
  $source=Join-Path $repo $_;$destination=Join-Path $sourceRoot $_;$hash=Hash $source
  New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force|Out-Null
  Copy-Item -LiteralPath $source -Destination $destination
  if((Hash $destination) -ne $hash){throw 'Source copy changed'}
  [ordered]@{path=$_;sha256=$hash}
})
$configure=$null;$compile=$null;$deployment=$null;$failure=$null
try {
  Write-Output ('Configure: writing '+(Join-Path $run 'configure.log'))
  & $CMake -S $sourceRoot -B $build -G 'Visual Studio 17 2022' -A Win32 '-DCMAKE_SYSTEM_VERSION=10.0.26100.0' -DPRODUCER_BUILD_REFERENCE_TOOLS=OFF -DPRODUCER_BUILD_CORE_TESTS=ON *> (Join-Path $run 'configure.log')
  $configure=$LASTEXITCODE;if($configure -ne 0){throw 'Product configuration failed'}
  Write-Output ('Configure completed. Build: writing '+(Join-Path $run 'build.log'))
  & $CMake --build $build --config Release *> (Join-Path $run 'build.log')
  $compile=$LASTEXITCODE;if($compile -ne 0){throw 'Product compilation failed'}
  Write-Output ('Build completed. Install: writing '+(Join-Path $run 'install.log'))
  & $CMake --install $build --config Release --prefix $install *> (Join-Path $run 'install.log')
  $deployment=$LASTEXITCODE;if($deployment -ne 0){throw 'Product install failed'}
  Write-Output 'Install completed.'
} catch {$failure=$_.Exception.Message}
$outputs=@()
foreach($path in @('build/Release/Producer.exe','build/Release/producer_core.lib','build/Release/producer_core_tests.exe','install/bin/Producer.exe')) {
  $file=Join-Path $run $path;if(Test-Path -LiteralPath $file){$outputs+=[ordered]@{path=$path;bytes=(Get-Item -LiteralPath $file).Length;sha256=(Hash $file)}}
}
$unchanged=@($sources|Where-Object {(Hash (Join-Path $sourceRoot $_.path)) -ne $_.sha256}).Count -eq 0
& $CMake --version *> (Join-Path $run 'cmake-version.txt')
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');sourceRoot=$sourceRoot;buildDirectory=$build;installDirectory=$install;sources=$sources;sourceSnapshotUnchanged=$unchanged;
  cmake=$CMake;cmakeSha256=(Hash $CMake);osVersion=[Environment]::OSVersion.VersionString;generator='Visual Studio 17 2022';architecture='Win32';configuration='Release';
  configureExitCode=$configure;buildExitCode=$compile;installExitCode=$deployment;error=$failure;outputs=$outputs;passed=($null -eq $failure -and $unchanged);runtimeExecuted=$false;hostAcceptance='unexecuted';
  dependencies=@('Windows system APIs','Windows XAudio2_9.dll reverb APO (Standard Environmental Reverb source runtime)','MSVC C++ runtime statically linked (/MT)');windowsSdk='10.0.26100.0';originalModulesRequired=$false;scope='Source-only product and core tests; no original files or COM registry required by configuration. Runtime module inventory and UI/audio acceptance separate.'
}|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $run 'build-summary.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)
if($failure){throw $failure}

[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath,[Parameter(Mandatory)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$basePath=[IO.Path]::GetFullPath($BuildSummaryPath);$base=Get-Content -LiteralPath $basePath -Raw|ConvertFrom-Json
if(-not $base.passed){throw 'Successful product library required'}
$dir=[IO.Path]::GetFullPath($EvidenceDirectory);$sources=Join-Path $dir 'sources';$build=Join-Path $dir 'build'
if(Test-Path -LiteralPath $dir){throw 'Fresh diagnostic directory required'}
New-Item -ItemType Directory -Path $sources|Out-Null
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$inventory=@($base.sources|ForEach-Object {
  $input=Join-Path $repo $_.path;$output=Join-Path $sources $_.path
  New-Item -ItemType Directory -Path (Split-Path $output -Parent) -Force|Out-Null
  Copy-Item -LiteralPath $input -Destination $output
  [ordered]@{path=$_.path;sha256=Hash $output;baseSha256=$_.sha256}
})
$library=Join-Path (Split-Path $basePath -Parent) 'build/Release/producer_core.lib'
$expected=@($base.outputs|Where-Object path -eq 'build/Release/producer_core.lib')[0]
if((Hash $library) -ne $expected.sha256){throw 'Saved library changed'}
# Diagnostics link the unchanged source product library. This is not a new
# product build/install or acceptance candidate; preserve the parent outputs.
foreach($s in $inventory|Where-Object {$_.path -like 'src/*' -and $_.path -notlike '*_editor.cpp' -and $_.path -ne 'src/producer/main.cpp'}){
  if($s.sha256 -ne $s.baseSha256){throw ('Library source mismatch: '+$s.path)}
}
$config=@'
cmake_minimum_required(VERSION 3.21)
project(SourceToolDiagnostic LANGUAGES CXX)
add_executable(source_tool_diagnostic tests/producer/core_tests.cpp src/producer/conductor.cpp)
target_compile_features(source_tool_diagnostic PRIVATE cxx_std_17)
target_include_directories(source_tool_diagnostic PRIVATE src)
target_compile_definitions(source_tool_diagnostic PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(source_tool_diagnostic PRIVATE "@LIBRARY@" oleaut32 uuid ole32 user32 advapi32)
set_property(TARGET source_tool_diagnostic PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_options(source_tool_diagnostic PRIVATE /W4 /utf-8)
'@
Copy-Item -LiteralPath (Join-Path $sources 'CMakeLists.txt') -Destination (Join-Path $sources 'product-CMakeLists.txt')
$config.Replace('@LIBRARY@',$library.Replace('\','/'))|Set-Content -LiteralPath (Join-Path $sources 'CMakeLists.txt') -Encoding UTF8
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
& $base.cmake -S $sources -B $build -G 'Visual Studio 17 2022' -A Win32 '-DCMAKE_SYSTEM_VERSION=10.0.26100.0' *> (Join-Path $dir 'configure.log');$configure=$LASTEXITCODE
$compile=$null
if($configure -eq 0){& $base.cmake --build $build --config Release *> (Join-Path $dir 'build.log');$compile=$LASTEXITCODE}
$exe=Join-Path $build 'Release/source_tool_diagnostic.exe'
$record=[ordered]@{schema=1;scope='Diagnostic test executable with saved matching product library; no product install/GUI/full acceptance';baseBuild=$basePath;baseBuildSha256=Hash $basePath;library=$library;librarySha256=Hash $library;sources=$inventory;diagnosticCMakeSha256=Hash (Join-Path $sources 'CMakeLists.txt');configureExitCode=$configure;compileExitCode=$compile;executable=$exe;exeSha256=$null;fullAcceptance=$false}
if(Test-Path -LiteralPath $exe){$record.exeSha256=Hash $exe}
$record|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dir 'build-summary.json') -Encoding UTF8
if($configure -ne 0 -or $compile -ne 0){throw 'Diagnostic compile failed'}
$out=Join-Path $dir 'runtime';New-Item -ItemType Directory -Path $out|Out-Null
& $exe $out --param-control-runtime *> (Join-Path $dir 'runtime.log');$code=$LASTEXITCODE
[ordered]@{exitCode=$code;executable=$exe;exeSha256=Hash $exe;arguments=@($out,'--param-control-runtime');log='runtime.log';scope=$record.scope;fullAcceptance=$false}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding UTF8
Write-Output ('Evidence: '+$dir)
if($code -ne 0){throw 'Diagnostic assertion failed; preserve actual note evidence'}

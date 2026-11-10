[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$root=Join-Path $repo ('work/build/audio-capture/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'));$source=Join-Path $root 'sources';New-Item -ItemType Directory -Path $source|Out-Null
$inputs=@('CMakeLists.txt','loopback.cpp')|ForEach-Object {Copy-Item -LiteralPath (Join-Path $repo ('tests/audio/'+$_)) -Destination $source;[ordered]@{path=$_;sha256=Hash (Join-Path $source $_)}}
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $root 'build-driver.ps1')
& cmake -S $source -B (Join-Path $root 'build') -A Win32 *> (Join-Path $root 'configure.log');$configure=$LASTEXITCODE
$compile=$null;if($configure -eq 0){& cmake --build (Join-Path $root 'build') --config Release *> (Join-Path $root 'build.log');$compile=$LASTEXITCODE}
$exe=Join-Path $root 'build/Release/producer_loopback.exe'
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');sources=$inputs;configureExitCode=$configure;compileExitCode=$compile;passed=($configure -eq 0 -and $compile -eq 0);executable=$exe;sha256=if(Test-Path -LiteralPath $exe){Hash $exe}else{$null};driverSha256=Hash $PSCommandPath;configureLogSha256=Hash (Join-Path $root 'configure.log');buildLogSha256=if(Test-Path -LiteralPath (Join-Path $root 'build.log')){Hash (Join-Path $root 'build.log')}else{$null};scope='Recorder configure/compile only; no product acceptance or capture execution'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $root 'build-summary.json') -Encoding UTF8
Write-Output $root
if($configure -ne 0 -or $compile -ne 0){throw 'Recorder build failed'}

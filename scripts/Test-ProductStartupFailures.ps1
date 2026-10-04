[CmdletBinding()]
param([Parameter(Mandatory)][string]$BuildSummaryPath)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
function Hash([string]$file){(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
$summaryPath=[IO.Path]::GetFullPath($BuildSummaryPath)
$summary=Get-Content -LiteralPath $summaryPath -Raw|ConvertFrom-Json
if(-not $summary.passed -or -not $summary.sourceSnapshotUnchanged){throw 'Successful saved build required'}
foreach($source in $summary.sources){if((Hash (Join-Path $summary.sourceRoot $source.path)) -ne $source.sha256){throw 'Saved source changed'}}
$output=@($summary.outputs|Where-Object path -eq 'install/bin/Producer.exe')
if($output.Count -ne 1){throw 'Installed executable missing'}
$exe=Join-Path (Split-Path $summaryPath -Parent) $output[0].path
if((Hash $exe) -ne $output[0].sha256){throw 'Executable changed'}
$dir=Join-Path $repo ('work/acceptance/product-startup-failures/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $dir|Out-Null
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $dir 'driver.ps1')
$invalid=Join-Path $dir 'invalid.dmpj';[IO.File]::WriteAllText($invalid,'invalid project fixture')
$invalidHash=Hash $invalid
$cases=@(
    @{name='missing-project-argument';args=@('--open-project')},
    @{name='unsupported-option';args=@('--unsupported-startup-option')},
    @{name='invalid-project';args=@('--open-project',('"'+$invalid+'"'))}
)
$results=@()
foreach($case in $cases){
    $process=Start-Process -FilePath $exe -ArgumentList $case.args -WindowStyle Hidden -PassThru
    $exited=$process.WaitForExit(5000)
    $exit=$null;if($exited){$process.Refresh();$exit=$process.ExitCode}
    $results+=[ordered]@{name=$case.name;arguments=$case.args;processId=$process.Id;exited=$exited;exitCode=$exit;passed=($exited -and $exit -eq 1)}
    if(-not $exited){break} # No forced termination or repeated launch on a blocked path.
}
$unchanged=(Hash $invalid) -eq $invalidHash
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');build=$summaryPath;buildSha256=(Hash $summaryPath);exe=$exe;exeSha256=$output[0].sha256;driverSha256=(Hash $PSCommandPath);invalidInput=@{path=$invalid;sha256=$invalidHash;unchanged=$unchanged};cases=$results;passed=($unchanged -and $results.Count -eq 3 -and @($results|Where-Object {-not $_.passed}).Count -eq 0);scope='Startup refusal exit codes and invalid input preservation; GUI/audio/full acceptance separate';fullAcceptancePassed=$false}
$record|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $dir 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$dir)
if(-not $record.passed){throw 'Startup refusal failed; see run.json'}

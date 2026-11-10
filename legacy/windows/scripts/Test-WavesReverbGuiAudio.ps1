[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$DryCapture,
    [Parameter(Mandatory)][string]$WetCapture,
    [Parameter(Mandatory)][string]$Protocol,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$NodeExecutable='C:\Users\dolph\Tools\node-v24.18.1-win-x64\node.exe'
)
$ErrorActionPreference='Stop'
function FileHash([string]$p){(Get-FileHash -LiteralPath $p).Hash.ToLowerInvariant()}
$waveAuditDirectory=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $waveAuditDirectory){throw 'Preserve existing audit directory'}
$null=New-Item -ItemType Directory -Path $waveAuditDirectory
$waveAuditRepo=Split-Path -Parent $PSScriptRoot
$waveAuditProtocol=[IO.Path]::GetFullPath($Protocol)
$waveAuditSteps=@(
    @{name='controls';script='Test-WavesReverbGuiAudioAuditor.mjs';arguments=@($waveAuditProtocol,(Join-Path $waveAuditDirectory 'controls.json'))},
    @{name='comparison';script='Inspect-WavesReverbGuiAudio.mjs';arguments=@([IO.Path]::GetFullPath($DryCapture),[IO.Path]::GetFullPath($WetCapture),$waveAuditProtocol,(Join-Path $waveAuditDirectory 'pcm-proof.json'))}
)
$waveAuditSources=@('Test-WavesReverbGuiAudio.ps1','Inspect-WavesReverbGuiAudio.mjs','Test-WavesReverbGuiAudioAuditor.mjs','AudioCaptureClock.mjs','Inspect-WavesReverbFixture.mjs')
$waveAuditSourceRecords=@($waveAuditSources|ForEach-Object {
    $waveAuditSource=Join-Path $PSScriptRoot $_
    Copy-Item -LiteralPath $waveAuditSource -Destination (Join-Path $waveAuditDirectory $_)
    [ordered]@{path=('scripts/'+$_);sha256=FileHash $waveAuditSource}
})
$waveAuditRecord=[ordered]@{schema=1;createdUtc=[datetime]::UtcNow.ToString('o');protocol=$waveAuditProtocol;protocolSha256=FileHash $waveAuditProtocol;node=$NodeExecutable;nodeSha256=FileHash $NodeExecutable;sources=$waveAuditSourceRecords;steps=@();passed=$false;scope='Supplemental GUI Waves dry/wet capture acceptance and independent PCM controls; registered full driver round remains separate';fullAcceptance=$false}
foreach($waveAuditStep in $waveAuditSteps){
    $waveAuditScript=Join-Path $PSScriptRoot $waveAuditStep.script
    $waveAuditArguments=@($waveAuditScript)+$waveAuditStep.arguments
    $waveAuditQuoted=@($waveAuditArguments|ForEach-Object {if($_ -match '"'){throw 'Unexpected quote in path'};'"'+$_+'"'})
    $waveAuditStarted=[datetime]::UtcNow.ToString('o')
    $waveAuditProcess=Start-Process -FilePath $NodeExecutable -ArgumentList $waveAuditQuoted -WorkingDirectory $waveAuditRepo -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $waveAuditDirectory ($waveAuditStep.name+'.stdout.txt')) -RedirectStandardError (Join-Path $waveAuditDirectory ($waveAuditStep.name+'.stderr.txt'))
    $waveAuditProcess.WaitForExit();$waveAuditProcess.Refresh()
    $waveAuditRecord.steps+= [ordered]@{name=$waveAuditStep.name;pid=$waveAuditProcess.Id;arguments=$waveAuditArguments;startedUtc=$waveAuditStarted;endedUtc=[datetime]::UtcNow.ToString('o');exitCode=$waveAuditProcess.ExitCode}
    $waveAuditRecord|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $waveAuditDirectory 'run.json') -Encoding utf8
    if($waveAuditProcess.ExitCode -ne 0){throw ('Waves GUI audio '+$waveAuditStep.name+' failed; preserve evidence')}
}
$waveAuditRecord.passed=$true
$waveAuditRecord|ConvertTo-Json -Depth 10|Set-Content -LiteralPath (Join-Path $waveAuditDirectory 'run.json') -Encoding utf8
Write-Output ('Evidence: '+$waveAuditDirectory)

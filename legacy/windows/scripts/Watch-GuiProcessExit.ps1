[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,[Parameter(Mandatory)][string]$Executable,[Parameter(Mandatory)][string]$ExpectedSha256,[Parameter(Mandatory)][string]$EvidencePath)
$ErrorActionPreference='Stop'
$full=[IO.Path]::GetFullPath($Executable)
$hash=(Get-FileHash -LiteralPath $full).Hash.ToLowerInvariant()
if($hash -ne $ExpectedSha256){throw 'Candidate executable identity mismatch'}
$process=Get-Process -Id $TargetProcessId
if($process.Path -ne $full){throw 'Target process path mismatch'}
$record=[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');processId=$TargetProcessId;executable=$full;executableSha256=$hash;handleAcquired=$false;exited=$false;exitCode=$null;error=$null;scope='Observed process termination only; GUI edit/save and audio require separate evidence';fullAcceptance=$false}
try {
  $process.EnableRaisingEvents=$true
  $null=$process.Handle
  $record.handleAcquired=$true
  $record|ConvertTo-Json|Set-Content -LiteralPath $EvidencePath -Encoding UTF8
  Write-Output 'Process handle acquired; waiting for normal GUI close'
  $process.WaitForExit()
  $record.exited=$true;$record.exitCode=$process.ExitCode;$record.finishedUtc=[DateTime]::UtcNow.ToString('o')
}catch {$record.error=$_.Exception.Message}
$record|ConvertTo-Json|Set-Content -LiteralPath $EvidencePath -Encoding UTF8
Write-Output ($record|ConvertTo-Json -Compress)
if($record.error){throw $record.error}

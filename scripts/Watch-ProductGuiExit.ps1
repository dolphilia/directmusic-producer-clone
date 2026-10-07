[CmdletBinding()]
param([Parameter(Mandatory)][string]$Executable,[Parameter(Mandatory)][ValidateRange(1,2147483647)][int]$ProcessId,[Parameter(Mandatory)][string]$OutputPath,[ValidateRange(0,900)][int]$TimeoutSeconds=120)
$ErrorActionPreference='Stop'
$exe=[IO.Path]::GetFullPath($Executable)
$process=Get-Process -Id $ProcessId
if($process.Path -ine $exe){throw 'Selected process executable mismatch'}
if(-not ('ProducerExitObserver' -as [type])){
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ProducerExitObserver {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,uint id);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern uint WaitForSingleObject(IntPtr handle,uint milliseconds);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool GetExitCodeProcess(IntPtr handle,out uint code);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] public static extern bool QueryFullProcessImageName(IntPtr handle,uint flags,StringBuilder name,ref uint size);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool CloseHandle(IntPtr handle);
}
'@
}
# Read-only process rights; keep the original handle open across GUI Close.
$handle=[ProducerExitObserver]::OpenProcess(0x101000,$false,[uint32]$ProcessId)
if($handle -eq [IntPtr]::Zero){throw ('OpenProcess refused: '+[Runtime.InteropServices.Marshal]::GetLastWin32Error())}
try {
    $name=[Text.StringBuilder]::new(32768);[uint32]$size=$name.Capacity
    if(-not [ProducerExitObserver]::QueryFullProcessImageName($handle,0,$name,[ref]$size)){throw 'Process image query failed'}
    if($name.ToString() -ine $exe){throw 'Native handle executable mismatch'}
    $record=[ordered]@{schema=1;processId=$ProcessId;executable=$exe;exeSha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();startUtc=$process.StartTime.ToUniversalTime().ToString('o');readyUtc=[DateTime]::UtcNow.ToString('o');state='watching';exitUtc=$null;exitCode=$null;forcedTermination=$false;driverSha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()}
    $record|ConvertTo-Json|Set-Content -LiteralPath $OutputPath -Encoding utf8
    # Explicit zero supports long authoring scenarios. Rights and forced-close
    # behavior remain unchanged; this observer never terminates the product.
    $deadline=if($TimeoutSeconds -gt 0){[DateTime]::UtcNow.AddSeconds($TimeoutSeconds)}else{$null}
    do {$status=[ProducerExitObserver]::WaitForSingleObject($handle,500);if($status -eq 0){break};if($status -ne 258){throw ('Process wait failed: '+$status)}} while($TimeoutSeconds -eq 0 -or [DateTime]::UtcNow -lt $deadline)
    if($status -eq 0){[uint32]$code=0;if(-not [ProducerExitObserver]::GetExitCodeProcess($handle,[ref]$code)){throw 'Native exit code query failed'};$record.state='exited';$record.exitCode=$code;$record.exitUtc=[DateTime]::UtcNow.ToString('o')}
    else {$record.state='timeout; process left running'}
    $record|ConvertTo-Json|Set-Content -LiteralPath $OutputPath -Encoding utf8
    if($status -ne 0){throw 'GUI process still running; no forced termination'}
    Write-Output ('GUI exit code: '+$code)
} finally {[ProducerExitObserver]::CloseHandle($handle)|Out-Null}

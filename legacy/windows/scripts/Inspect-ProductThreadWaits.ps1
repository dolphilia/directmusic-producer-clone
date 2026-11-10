[CmdletBinding()]
param([Parameter(Mandatory)][int]$ProcessId,[Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference='Stop'
# Read-only wait-chain diagnostics: no debugger attachment or thread suspension.
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ProducerWaitInspector {
 [DllImport("advapi32.dll",SetLastError=true)] public static extern IntPtr OpenThreadWaitChainSession(uint flags,IntPtr callback);
 [DllImport("advapi32.dll")] public static extern void CloseThreadWaitChainSession(IntPtr session);
 [DllImport("advapi32.dll",SetLastError=true)] public static extern bool GetThreadWaitChain(IntPtr session,UIntPtr context,uint flags,uint thread,ref uint count,[Out] byte[] nodes,out bool cycle);
}
'@
$process=Get-Process -Id $ProcessId
$session=[ProducerWaitInspector]::OpenThreadWaitChainSession(0,[IntPtr]::Zero)
if($session -eq [IntPtr]::Zero){throw ('Wait-chain session refused: '+[Runtime.InteropServices.Marshal]::GetLastWin32Error())}
try {
 $threads=@(foreach($thread in $process.Threads){
  [uint32]$count=16;$nodes=New-Object byte[] (16*280);$cycle=$false
  $ok=[ProducerWaitInspector]::GetThreadWaitChain($session,[UIntPtr]::Zero,7,[uint32]$thread.Id,[ref]$count,$nodes,[ref]$cycle)
  $errorCode=if($ok){0}else{[Runtime.InteropServices.Marshal]::GetLastWin32Error()}
  $chain=@(if($ok){for($i=0;$i -lt $count;$i++){
   $offset=$i*280;$type=[BitConverter]::ToUInt32($nodes,$offset)
   $node=[ordered]@{objectType=$type;objectStatus=[BitConverter]::ToUInt32($nodes,$offset+4)}
   if($type -eq 8){$node.processId=[BitConverter]::ToUInt32($nodes,$offset+8);$node.threadId=[BitConverter]::ToUInt32($nodes,$offset+12);$node.waitTime=[BitConverter]::ToUInt32($nodes,$offset+16);$node.contextSwitches=[BitConverter]::ToUInt32($nodes,$offset+20)}
   else{$node.objectName=[Text.Encoding]::Unicode.GetString($nodes,$offset+8,256).TrimEnd([char]0)}
   $node
  }})
  [ordered]@{threadId=$thread.Id;startAddress=$thread.StartAddress.ToString();state=[string]$thread.ThreadState;totalCpuSeconds=$thread.TotalProcessorTime.TotalSeconds;queryPassed=$ok;errorCode=$errorCode;cycle=$cycle;chain=$chain}
 })
 $record=[ordered]@{schema=1;utc=[DateTime]::UtcNow.ToString('o');processId=$ProcessId;executable=$process.Path;threads=$threads;scope='Read-only wait chains; no stack attribution or exit acceptance';fullAcceptance=$false}
 $record|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $OutputPath -Encoding UTF8
 $record|ConvertTo-Json -Depth 8
} finally {[ProducerWaitInspector]::CloseThreadWaitChainSession($session)}

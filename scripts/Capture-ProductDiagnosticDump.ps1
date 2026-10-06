[CmdletBinding()]
param([Parameter(Mandatory)][int]$ProcessId,[Parameter(Mandatory)][string]$OutputPath)
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ProducerDumpReader {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint rights,bool inherit,uint process);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
 [DllImport("dbghelp.dll",SetLastError=true)] public static extern bool MiniDumpWriteDump(IntPtr process,uint id,IntPtr file,uint type,IntPtr exception,IntPtr streams,IntPtr callback);
}
'@
$process=Get-Process -Id $ProcessId
$handle=[ProducerDumpReader]::OpenProcess(0x410,$false,[uint32]$ProcessId)
if($handle -eq [IntPtr]::Zero){throw ('Diagnostic read refused: '+[Runtime.InteropServices.Marshal]::GetLastWin32Error())}
$file=$null
try {
 $file=[IO.File]::Open([IO.Path]::GetFullPath($OutputPath),[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
 # Ordinary minidump + thread metadata; omit full process memory.
 if(-not [ProducerDumpReader]::MiniDumpWriteDump($handle,[uint32]$ProcessId,$file.SafeFileHandle.DangerousGetHandle(),0x1000,[IntPtr]::Zero,[IntPtr]::Zero,[IntPtr]::Zero)){throw ('Diagnostic dump refused: '+[Runtime.InteropServices.Marshal]::GetLastWin32Error())}
} finally {if($file){$file.Dispose()};[void][ProducerDumpReader]::CloseHandle($handle)}
[ordered]@{processId=$ProcessId;executable=$process.Path;utc=[DateTime]::UtcNow.ToString('o');dump=[IO.Path]::GetFullPath($OutputPath);sha256=(Get-FileHash -LiteralPath $OutputPath).Hash.ToLowerInvariant();scope='Read-only ordinary minidump with thread metadata, without full process memory';fullAcceptance=$false}|ConvertTo-Json|Set-Content -LiteralPath ($OutputPath+'.json') -Encoding UTF8

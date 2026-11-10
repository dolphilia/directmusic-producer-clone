[CmdletBinding()]
param([Parameter(Mandatory)][string]$Path,[Parameter(Mandatory)][string]$Output)
$ErrorActionPreference='Stop'
Add-Type -TypeDefinition @'
using System;using System.Text;using System.Runtime.InteropServices;
public static class RuntimeFileLockReadOnly {
 [StructLayout(LayoutKind.Sequential)] public struct Unique { public uint Id; public System.Runtime.InteropServices.ComTypes.FILETIME Start; }
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct Info { public Unique Process; [MarshalAs(UnmanagedType.ByValTStr,SizeConst=256)] public string App; [MarshalAs(UnmanagedType.ByValTStr,SizeConst=64)] public string Service; public uint Type,Status,Session; [MarshalAs(UnmanagedType.Bool)] public bool Restartable; }
 [DllImport("rstrtmgr.dll",CharSet=CharSet.Unicode)] public static extern uint RmStartSession(out uint handle,uint flags,StringBuilder key);
 [DllImport("rstrtmgr.dll",CharSet=CharSet.Unicode)] public static extern uint RmRegisterResources(uint handle,uint count,string[] files,uint apps,IntPtr processes,uint services,IntPtr names);
 [DllImport("rstrtmgr.dll")] public static extern uint RmGetList(uint handle,out uint needed,ref uint count,[In,Out] Info[] info,out uint reasons);
 [DllImport("rstrtmgr.dll")] public static extern uint RmEndSession(uint handle);
}
'@
$full=[IO.Path]::GetFullPath($Path);[uint32]$handle=0;$key=[Text.StringBuilder]::new(33);$start=[RuntimeFileLockReadOnly]::RmStartSession([ref]$handle,0,$key);$register=$null;$first=$null;$second=$null;$end=$null;$items=@()
try {if($start -eq 0){$register=[RuntimeFileLockReadOnly]::RmRegisterResources($handle,1,@($full),0,[IntPtr]::Zero,0,[IntPtr]::Zero);if($register -eq 0){[uint32]$needed=0;[uint32]$count=0;[uint32]$reasons=0;$first=[RuntimeFileLockReadOnly]::RmGetList($handle,[ref]$needed,[ref]$count,$null,[ref]$reasons);if($first -eq 234 -and $needed -gt 0){$data=[RuntimeFileLockReadOnly+Info[]]::new($needed);$count=$needed;$second=[RuntimeFileLockReadOnly]::RmGetList($handle,[ref]$needed,[ref]$count,$data,[ref]$reasons);if($second -eq 0){$items=@($data | Select-Object @{n='processId';e={$_.Process.Id}},App,Service,Type,Status,Session,Restartable)}}}}}
finally {if($start -eq 0){$end=[RuntimeFileLockReadOnly]::RmEndSession($handle)}}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');path=$full;readOnly=$true;file=(Get-Item -LiteralPath $full | Select-Object Attributes,Length,CreationTimeUtc,LastWriteTimeUtc);start=$start;register=$register;firstGetList=$first;secondGetList=$second;end=$end;resourceUsers=$items;scope='Current Restart Manager resource-user snapshot only. No shutdown/restart/filter/removal or OS setting change; no proof of holder at earlier failure time.'}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $Output -Encoding utf8
Write-Output ('Evidence: '+[IO.Path]::GetFullPath($Output))

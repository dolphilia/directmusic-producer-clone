[CmdletBinding()]
param([Parameter(Mandatory)][string[]]$Files,[Parameter(Mandatory)][string]$Output)
$ErrorActionPreference='Stop'
$repo=[IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$targets=@($Files | ForEach-Object {
  $full=[IO.Path]::GetFullPath($_)
  if(-not $full.StartsWith(($repo+'\work\'),[StringComparison]::OrdinalIgnoreCase)){throw 'Only this repository owned work evidence may be inspected'}
  if(-not (Test-Path -LiteralPath $full -PathType Leaf)){throw 'Owned evidence file missing'}
  $full
})
if(-not ('ProducerOwnedLockInspection' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class ProducerOwnedLockInspection {
  [StructLayout(LayoutKind.Sequential)] public struct FileTime { public uint Low,High; }
  [StructLayout(LayoutKind.Sequential)] public struct ProcessIdentity { public uint Id; public FileTime Start; }
  [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] public struct ProcessInfo {
    public ProcessIdentity Process;
    [MarshalAs(UnmanagedType.ByValTStr,SizeConst=256)] public string App;
    [MarshalAs(UnmanagedType.ByValTStr,SizeConst=64)] public string Service;
    public uint Type,Status,Session;
    [MarshalAs(UnmanagedType.Bool)] public bool Restartable;
  }
  public sealed class Result { public uint Start,Register,Query,End; public ProcessInfo[] Owners; }
  [DllImport("rstrtmgr.dll",CharSet=CharSet.Unicode)] static extern uint RmStartSession(out uint handle,uint flags,StringBuilder key);
  [DllImport("rstrtmgr.dll",CharSet=CharSet.Unicode)] static extern uint RmRegisterResources(uint handle,uint files,string[] names,uint apps,IntPtr processes,uint services,IntPtr serviceNames);
  [DllImport("rstrtmgr.dll")] static extern uint RmGetList(uint handle,out uint needed,ref uint count,[In,Out] ProcessInfo[] owners,out uint reboot);
  [DllImport("rstrtmgr.dll")] static extern uint RmEndSession(uint handle);
  public static Result Inspect(string path) {
    var r=new Result(); uint handle; r.Start=RmStartSession(out handle,0,new StringBuilder(33));
    r.Owners=new ProcessInfo[0]; if(r.Start!=0)return r;
    try {
      r.Register=RmRegisterResources(handle,1,new[]{path},0,IntPtr.Zero,0,IntPtr.Zero);if(r.Register!=0)return r;
      uint count=0,needed,reboot;r.Query=RmGetList(handle,out needed,ref count,null,out reboot);
      if(r.Query==0)return r;if(r.Query!=234)return r;
      for(int i=0;i<3;i++){var a=new ProcessInfo[needed];count=needed;r.Query=RmGetList(handle,out needed,ref count,a,out reboot);if(r.Query==0){Array.Resize(ref a,(int)count);r.Owners=a;break;}if(r.Query!=234)break;}
    } finally {r.End=RmEndSession(handle);}
    return r;
  }
}
'@
}
$rmUnavailable=$null
$results=@($targets | ForEach-Object {
  $before=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant();$item=Get-Item -LiteralPath $_
  if($null -eq $rmUnavailable){$r=[ProducerOwnedLockInspection]::Inspect($_);if($r.Start -ne 0){$rmUnavailable=$r.Start}}
  else {$r=$null}
  [ordered]@{path=$_;sha256=$before;bytes=$item.Length;attributes=$item.Attributes.ToString();inspectionUtc=[DateTime]::UtcNow.ToString('o');startCode=if($r){$r.Start}else{$null};registerCode=if($r -and $r.Start -eq 0){$r.Register}else{$null};queryCode=if($r -and $r.Start -eq 0 -and $r.Register -eq 0){$r.Query}else{$null};endCode=if($r -and $r.Start -eq 0){$r.End}else{$null};status=if($r -and $r.Start -eq 0 -and $r.Register -eq 0 -and $r.Query -eq 0){'観測完了'}else{'障害あり'};startupObstacle=$rmUnavailable;
    owners=@($r.Owners | ForEach-Object {[ordered]@{processId=$_.Process.Id;name=$_.App;service=$_.Service;applicationType=$_.Type;status=$_.Status;startTimeLow=$_.Process.Start.Low;startTimeHigh=$_.Process.Start.High}});
    unchanged=((Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant() -eq $before)}
})
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');results=$results;scope='Read-only current Restart Manager owner inventory for owned evidence. No shutdown/restart/termination. A missing current owner does not establish the cause of a historical refusal.';driverSha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()} | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath $Output -Encoding UTF8
Write-Output ('Evidence: '+$Output)

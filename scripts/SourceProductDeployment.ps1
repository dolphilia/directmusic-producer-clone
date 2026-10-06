#Requires -Version 7.2
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$ProductOwner='DirectMusicProducer.Source.v1'
if(-not ('SourceProduct.PathMetadata' -as [type])){Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace SourceProduct {
 public static class PathMetadata {
  [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct Data {
   public uint attributes; public System.Runtime.InteropServices.ComTypes.FILETIME creation,access,write;
   public uint sizeHigh,sizeLow,tag,reserved;
   [MarshalAs(UnmanagedType.ByValTStr,SizeConst=260)] public string name;
   [MarshalAs(UnmanagedType.ByValTStr,SizeConst=14)] public string alternate;
  }
  [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] static extern IntPtr FindFirstFileW(string path,out Data data);
  [DllImport("kernel32.dll",SetLastError=true)] static extern bool FindClose(IntPtr handle);
  public static uint Tag(string path) {Data d;var h=FindFirstFileW(path,out d);if(h==new IntPtr(-1))throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());try{return (d.attributes&0x400)!=0?d.tag:0;}finally{FindClose(h);}}
 }
}
'@}
function Test-ProductReparseTag([uint32]$Tag){return $Tag -eq 0 -or ($Tag -band [uint32]4294905855) -eq [uint32]2415919130}
function Assert-ProductEntry([IO.FileSystemInfo]$Entry){if(($Entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0){$tag=[SourceProduct.PathMetadata]::Tag($Entry.FullName);if(-not(Test-ProductReparseTag $tag)){throw ('Redirecting/unknown reparse tag refused: 0x'+$tag.ToString('x8'))}}}

$ProductFiles=@('bin/Producer.exe','tools/SourceProductDeployment.ps1','tools/Install-SourceProduct.ps1','tools/Uninstall-SourceProduct.ps1','docs/source-product-deployment.md')
function Get-ProductHash([string]$Path) {(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Get-PlainProductPath([string]$Path) {
 $full=[IO.Path]::GetFullPath($Path);$volume=[IO.Path]::GetPathRoot($full)
 if($full.TrimEnd('\','/') -eq $volume.TrimEnd('\','/')){throw 'A volume root is not a product directory'}
 $cursor=$full
 while($cursor){if(Test-Path -LiteralPath $cursor){$entry=Get-Item -LiteralPath $cursor -Force;Assert-ProductEntry $entry};$parent=[IO.Path]::GetDirectoryName($cursor);if($parent -eq $cursor){break};$cursor=$parent}
 return $full.TrimEnd('\','/')
}
function Get-ProductChild([string]$Root,[string]$Relative) {
 if($ProductFiles -notcontains $Relative){throw 'Undeclared product file'}
 $path=[IO.Path]::GetFullPath((Join-Path $Root $Relative));if(-not $path.StartsWith($Root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Product file escapes root'}
 return Get-PlainProductPath $path
}
function Assert-ProductTree([string]$Root,[string[]]$Expected) {
 $items=@(Get-ChildItem -LiteralPath $Root -Force -Recurse)
 $allowedDirectories=@('bin','tools','docs')
 foreach($item in $items){Assert-ProductEntry $item;$rel=[IO.Path]::GetRelativePath($Root,$item.FullName).Replace('\','/');if($item.PSIsContainer){if($allowedDirectories -notcontains $rel){throw ('Unlisted directory: '+$rel)}}elseif($Expected -notcontains $rel){throw ('Unlisted file: '+$rel)}}
 foreach($rel in $Expected){if(-not(Test-Path -LiteralPath (Join-Path $Root $rel) -PathType Leaf)){throw ('Missing product file: '+$rel)}}
}

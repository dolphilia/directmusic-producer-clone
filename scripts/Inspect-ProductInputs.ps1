[CmdletBinding()]
param([string[]]$InputPaths=@('work/producer/samples/QuickStart/QuickStart.pro','work/producer/samples/QuickStart/heartland.sgp'))
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$run=Join-Path $repo ('work/analysis/product-inputs/'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
New-Item -ItemType Directory -Path $run|Out-Null
$scriptHash=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()
Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $run 'Inspect-ProductInputs.ps1')
function Inspect-Chunks([byte[]]$bytes,[long]$start,[long]$end,[string]$parent,[int]$depth) {
  if($depth -gt 64){throw 'RIFF nesting limit'}
  $counts=@{}
  for($at=$start;$at -lt $end;) {
    if($end-$at -lt 8){throw 'Truncated chunk'}
    $id=[Text.Encoding]::ASCII.GetString($bytes,$at,4);$size=[BitConverter]::ToUInt32($bytes,$at+4);$data=$at+8
    if($size -gt $end-$data){throw 'Chunk exceeds container'}
    $stop=$data+$size;$kind=$null
    if($id -eq 'RIFF' -or $id -eq 'LIST'){if($size -lt 4){throw 'Container type missing'};$kind=[Text.Encoding]::ASCII.GetString($bytes,$data,4)}
    $label=if($kind){$id+':'+$kind}else{$id};$index=if($counts.ContainsKey($label)){$counts[$label]}else{0};$counts[$label]=$index+1
    $location=$parent+'/'+$label+'['+$index+']'
    $payload=New-Object byte[] $size;[Array]::Copy($bytes,$data,$payload,0,$size)
    $sha=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($payload)).ToLowerInvariant()
    $details=$null
    if($id -eq 'tetr') {
      if($size -lt 4){throw 'Tempo record size missing'};$record=[BitConverter]::ToUInt32($payload,0)
      if($record -ne 16 -or ($size-4)%16){throw 'Unsupported tempo layout'}
      $details=@(for($p=4;$p -lt $size;$p+=16){[ordered]@{time=[BitConverter]::ToInt32($payload,$p);bpm=[BitConverter]::ToDouble($payload,$p+8)}})
    } elseif($id -eq 'trkh' -and $size -ge 32) {
      $guidBytes=New-Object byte[] 16;[Array]::Copy($payload,0,$guidBytes,0,16)
      $details=[ordered]@{classId=([Guid]::new($guidBytes)).ToString();position=[BitConverter]::ToUInt32($payload,16);groups=[BitConverter]::ToUInt32($payload,20);chunk=[Text.Encoding]::ASCII.GetString($payload,24,4);list=[Text.Encoding]::ASCII.GetString($payload,28,4)}
    } elseif($id -eq 'segh' -and $size -ge 24) {
      $details=[ordered]@{repeats=[BitConverter]::ToUInt32($payload,0);length=[BitConverter]::ToInt32($payload,4);playStart=[BitConverter]::ToInt32($payload,8);loopStart=[BitConverter]::ToInt32($payload,12);loopEnd=[BitConverter]::ToInt32($payload,16);resolution=[BitConverter]::ToUInt32($payload,20)}
    } elseif($id -eq 'name' -and $parent -match '/LIST:file\[') {$details=[Text.Encoding]::Unicode.GetString($payload).TrimEnd([char]0)}
    [ordered]@{path=$location;id=$id;type=$kind;offset=$at;size=$size;payloadSha256=$sha;details=$details}
    if($kind){Inspect-Chunks $bytes ($data+4) $stop $location ($depth+1)}
    $at=$stop+($size%2);if($at -gt $end){throw 'Padding missing'}
  }
}
$reports=@();$number=0
foreach($inputPath in $InputPaths) {
  $file=if([IO.Path]::IsPathRooted($inputPath)){$inputPath}else{Join-Path $repo $inputPath}
  $bytes=[IO.File]::ReadAllBytes($file);if($bytes.Length -lt 12 -or $bytes.Length -gt 64MB -or [Text.Encoding]::ASCII.GetString($bytes,0,4) -ne 'RIFF'){throw 'Expected bounded RIFF input'}
  if([BitConverter]::ToUInt32($bytes,4)+8 -ne $bytes.Length){throw 'Root length mismatch'}
  $hash=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant();$name=($number.ToString()+'-'+[IO.Path]::GetFileName($file));$number++
  Copy-Item -LiteralPath $file -Destination (Join-Path $run $name)
  $reports+=[ordered]@{path=[IO.Path]::GetFullPath($file);savedInput=$name;sha256=$hash;bytes=$bytes.Length;chunks=@(Inspect-Chunks $bytes 0 $bytes.Length '' 0)}
}
[ordered]@{schema=1;createdUtc=[DateTime]::UtcNow.ToString('o');scriptSha256=$scriptHash;reports=$reports;scope='Read-only static format observation. Does not execute or validate the reconstructed application.'}|ConvertTo-Json -Depth 12|Set-Content -LiteralPath (Join-Path $run 'inputs.json') -Encoding UTF8
Write-Output ('Evidence: '+$run)

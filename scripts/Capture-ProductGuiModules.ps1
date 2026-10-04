[CmdletBinding()]
param([Parameter(Mandatory)][string]$Executable,[Parameter(Mandatory)][string]$OutputPath,[ValidateRange(1,2147483647)][int]$ProcessId)
$ErrorActionPreference='Stop'
$exe=[IO.Path]::GetFullPath($Executable)
if($PSBoundParameters.ContainsKey('ProcessId')){
    $processes=@(Get-Process -Id $ProcessId | Where-Object Path -eq $exe)
    if($processes.Count -ne 1){throw 'Selected process does not match the recorded GUI executable'}
}else{
    $processes=@(Get-Process -Name Producer | Where-Object Path -eq $exe)
}
if($processes.Count -ne 1){throw 'Expected one current GUI process'}
$entries=@($processes[0].Modules | ForEach-Object { [ordered]@{reportedPath=$_.FileName;baseAddressHex=$_.BaseAddress.ToInt64().ToString('x16');memorySize=$_.ModuleMemorySize} })
[ordered]@{schema=2;createdUtc=[DateTime]::UtcNow.ToString('o');executable=$exe;processId=$processes[0].Id;entries=$entries;paths=@($entries | ForEach-Object reportedPath);error=$null;auditorSha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant()} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
Write-Output ('Captured '+$entries.Count+' modules with base addresses')

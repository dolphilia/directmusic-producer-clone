; Scoped trial navigation through official AutoIt ControlTreeView.
; Arguments: PID, expected EXE, item path, expected text, observe|expand|open|properties, JSONL path.
#NoTrayIcon
#include <WinAPIProc.au3>
#include <WinAPISys.au3>
If $CmdLine[0]<>6 Then Exit 2
Global $gPid=Number($CmdLine[1]),$gExe=$CmdLine[2],$gItem=$CmdLine[3],$gText=$CmdLine[4],$gMode=$CmdLine[5]
Global $gLog=FileOpen($CmdLine[6],2+128)
If $gLog=-1 Then Exit 2
Opt("WinTitleMatchMode",3)
Func J($s)
    $s=StringReplace($s,'\','\\')
    $s=StringReplace($s,'"','\"')
    $s=StringReplace($s,@CR,'\r')
    $s=StringReplace($s,@LF,'\n')
    Return '"' & $s & '"'
EndFunc
Func LogLine($s)
    FileWrite($gLog,$s & @LF)
    FileFlush($gLog)
EndFunc
Func Fail($s)
    LogLine('{"operation":"failure","reason":' & J($s) & '}')
    FileClose($gLog)
    Exit 1
EndFunc
If $gMode<>"observe" And $gMode<>"expand" And $gMode<>"open" And $gMode<>"properties" Then Fail("Unknown action")
If StringLower(_WinAPI_GetProcessFileName($gPid))<>StringLower($gExe) Then Fail("Wrong process")
Local $windows=WinList(),$count=0,$frame=0,$tree=0
For $i=1 To $windows[0][0]
    Local $h=$windows[$i][1]
    If WinGetProcess($h)<>$gPid Or Not _WinAPI_IsWindowVisible($h) Or Not _WinAPI_IsWindowEnabled($h) Then ContinueLoop
    If WinGetTitle($h)<>"DirectMusic Producer" Or _WinAPI_GetClassName($h)<>"JzApBR" Then ContinueLoop
    Local $candidateTree=ControlGetHandle($h,"","SysTreeView321")
    If Not $candidateTree Or _WinAPI_GetDlgCtrlID($candidateTree)<>1000 Then ContinueLoop
    $frame=$h
    $tree=$candidateTree
    $count+=1
Next
If $count<>1 Then Fail("Expected one trial frame with project tree")
Local $text=ControlTreeView($frame,"",$tree,"GetText",$gItem)
If @error Or $text<>$gText Then Fail("Tree item text mismatch")
LogLine('{"operation":"tree_item","pid":' & $gPid & ',"path":' & J($gItem) & ',"text":' & J($text) & ',"children":' & ControlTreeView($frame,"",$tree,"GetItemCount",$gItem) & '}')
If $gMode<>"observe" Then
    If StringLower(_WinAPI_GetProcessFileName($gPid))<>StringLower($gExe) Then Fail("Process changed")
    WinActivate($frame)
    ControlFocus($frame,"",$tree)
    ControlSend($frame,"",$tree,"{ESC}")
    ControlTreeView($frame,"",$tree,"Select",$gItem)
    If @error Then Fail("Cannot select tree item")
    If $gMode="expand" Then
        ControlTreeView($frame,"",$tree,"Expand",$gItem)
        If @error Then Fail("Cannot expand tree item")
    ElseIf $gMode="open" Then
        If Not ControlSend($frame,"",$tree,"{ENTER}") Then Fail("Cannot open selected item")
    ElseIf $gMode="properties" Then
        If Not ControlSend($frame,"",$tree,"{F11}") Then Fail("Cannot request item properties")
    EndIf
    LogLine('{"operation":"tree_action","action":' & J($gMode) & ',"selected":' & J(ControlTreeView($frame,"",$tree,"GetSelected",1)) & '}')
EndIf
Local $children=ControlTreeView($frame,"",$tree,"GetItemCount",$gItem)
For $i=0 To $children-1
    Local $child=$gItem & "|#" & $i
    LogLine('{"operation":"tree_child","path":' & J($child) & ',"text":' & J(ControlTreeView($frame,"",$tree,"GetText",$child)) & '}')
Next
FileClose($gLog)
Exit 0

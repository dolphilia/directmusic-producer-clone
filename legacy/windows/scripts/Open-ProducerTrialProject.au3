; Official AutoIt interpreter. Only opens a .pro inside this trial's UiTest.
; Arguments: PID, expected EXE, project path, observe|open, PNG path, JSONL path.
#NoTrayIcon
#include <ScreenCapture.au3>
#include <WinAPIProc.au3>
#include <WinAPISys.au3>
If $CmdLine[0] <> 6 Then Exit 2
Global $gPid=Number($CmdLine[1]), $gExe=$CmdLine[2], $gProject=$CmdLine[3], $gMode=$CmdLine[4]
Global $gLog=FileOpen($CmdLine[6],2+128)
If $gLog=-1 Then Exit 2
Local $dpi=DllCall("user32.dll","handle","SetThreadDpiAwarenessContext","handle",-4)
If @error Or Not $dpi[0] Then Exit 1
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
Func Valid($h)
    Return WinGetProcess($h)=$gPid And StringLower(_WinAPI_GetProcessFileName($gPid))=StringLower($gExe) And _
        WinGetTitle($h)="Open Project" And _WinAPI_GetClassName($h)="#32770" And _WinAPI_IsWindowVisible($h) And _WinAPI_IsWindowEnabled($h)
EndFunc
If $gMode<>"observe" And $gMode<>"open" Then Fail("Unknown mode")
Local $prefix=StringLeft($gExe,StringInStr($gExe,"\",0,-1)) & "UiTest\"
If StringLeft(StringLower($gProject),StringLen($prefix))<>StringLower($prefix) Or StringInStr($gProject,"..") Or _
    StringRight(StringLower($gProject),4)<>".pro" Or Not FileExists($gProject) Then Fail("Expected an existing trial UiTest project")
Local $windows=WinList(),$count=0,$dialog=0
For $i=1 To $windows[0][0]
    If Valid($windows[$i][1]) Then
        $dialog=$windows[$i][1]
        $count+=1
    EndIf
Next
LogLine('{"operation":"open_dialog_count","pid":' & $gPid & ',"count":' & $count & '}')
If $count<>1 Then Fail("Expected exactly one owned Open Project dialog")
Local $edit=ControlGetHandle($dialog,"","[ID:1152]"),$button=0,$buttonCount=0
For $bi=1 To 4
    Local $candidateButton=ControlGetHandle($dialog,"","Button" & $bi)
    If $candidateButton Then LogLine('{"operation":"button_candidate","classNN":' & J("Button" & $bi) & ',"handle":' & Number($candidateButton) & ',"id":' & _WinAPI_GetDlgCtrlID($candidateButton) & ',"text":' & J(ControlGetText($dialog,"",$candidateButton)) & '}')
    If $candidateButton And _WinAPI_GetDlgCtrlID($candidateButton)=1 And _WinAPI_GetParent($candidateButton)=$dialog Then
        $button=$candidateButton
        $buttonCount+=1
    EndIf
Next
LogLine('{"operation":"file_controls","edit":' & Number($edit) & ',"editClass":' & J(_WinAPI_GetClassName($edit)) & ',"editParent":' & Number(_WinAPI_GetParent($edit)) & ',"dialog":' & Number($dialog) & ',"button":' & Number($button) & ',"buttonClass":' & J(_WinAPI_GetClassName($button)) & ',"buttonParent":' & Number(_WinAPI_GetParent($button)) & '}')
If Not $edit Or Not $button Or $buttonCount<>1 Or _WinAPI_GetClassName($edit)<>"Edit" Or _WinAPI_GetClassName($button)<>"Button" Or _
    _WinAPI_GetParent($edit)<>$dialog Or _WinAPI_GetParent($button)<>$dialog Or _WinAPI_GetDlgCtrlID($button)<>1 Then Fail("Unexpected file dialog controls")
WinActivate($dialog)
If Not WinWaitActive($dialog,"",3) Then Fail("Cannot activate verified file dialog")
If Not _ScreenCapture_CaptureWnd($CmdLine[5],$dialog,0,0,-1,-1,False) Then Fail("Cannot capture file dialog")
LogLine('{"operation":"open_dialog_snapshot","handle":' & Number($dialog) & ',"editId":1152,"buttonId":1,"buttonText":' & J(ControlGetText($dialog,"",$button)) & ',"screenshot":' & J($CmdLine[5]) & ',"captured":true}')
If $gMode="open" Then
    If Not Valid($dialog) Or Not _WinAPI_IsWindowEnabled($edit) Or Not _WinAPI_IsWindowEnabled($button) Then Fail("File dialog changed")
    If Not ControlSetText($dialog,"",$edit,$gProject) Or ControlGetText($dialog,"",$edit)<>$gProject Then Fail("Project path was not set exactly")
    LogLine('{"operation":"explicit_project_path","path":' & J(ControlGetText($dialog,"",$edit)) & '}')
    If Not Valid($dialog) Then Fail("File dialog changed before open")
    Local $clicked=ControlClick($dialog,"",$button,"left",1),$closed=WinWaitClose($dialog,"",5)
    LogLine('{"operation":"open_project","result":' & $clicked & ',"dialogDismissed":' & StringLower(String($closed<>0)) & '}')
    If $clicked<>1 Or Not $closed Then
        FileClose($gLog)
        Exit 1
    EndIf
EndIf
FileClose($gLog)
Exit 0

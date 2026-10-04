; Run with the official, signed AutoIt interpreter. No compilation or COM registration.
; Arguments: PID, expected EXE path, PNG path, observe|click, UTF-8 JSONL path.
#NoTrayIcon
#include <ScreenCapture.au3>
#include <WinAPIProc.au3>
#include <WinAPISys.au3>

If $CmdLine[0] <> 5 Then Exit 2
Global $gPid = Number($CmdLine[1]), $gExe = $CmdLine[2], $gPng = $CmdLine[3], $gMode = $CmdLine[4]
If $gPid <= 0 Or ($gMode <> "observe" And $gMode <> "click") Then Exit 2
Global $gLog = FileOpen($CmdLine[5], 2 + 128)
If $gLog = -1 Then Exit 2
; Keep GetWindowRect and screen capture in physical pixels on scaled displays.
Local $dpi = DllCall("user32.dll", "handle", "SetThreadDpiAwarenessContext", "handle", -4)
If @error Or Not $dpi[0] Then
    FileWrite($gLog, '{"operation":"failure","reason":"Cannot establish physical-pixel DPI context","clicked":false}' & @LF)
    FileClose($gLog)
    Exit 1
EndIf
Opt("WinTitleMatchMode", 3)
Opt("WinDetectHiddenText", 0)
Opt("WinWaitDelay", 100)

Func J($s)
    $s = StringReplace($s, '\', '\\')
    $s = StringReplace($s, '"', '\"')
    $s = StringReplace($s, @CR, '\r')
    $s = StringReplace($s, @LF, '\n')
    $s = StringReplace($s, @TAB, '\t')
    Return '"' & $s & '"'
EndFunc
Func LogLine($s)
    FileWrite($gLog, $s & @LF)
    FileFlush($gLog)
EndFunc
Func Fail($reason)
    LogLine('{"operation":"failure","reason":' & J($reason) & ',"clicked":false}')
    FileClose($gLog)
    Exit 1
EndFunc
Func Body($h)
    Local $classes = StringSplit(StringStripCR(WinGetClassList($h)), @LF, 2), $count = 0, $body = "", $texts = 0
    For $class In $classes
        If $class = "Static" Then $count += 1
    Next
    For $i = 1 To $count
        Local $s = ControlGetText($h, "", "Static" & $i)
        If $s <> "" Then
            $body = StringStripCR($s)
            $texts += 1
        EndIf
    Next
    If $texts <> 1 Then Return ""
    Return $body
EndFunc
Func Valid($h, ByRef $button, ByRef $body)
    If WinGetProcess($h) <> $gPid Or _WinAPI_GetClassName($h) <> "#32770" Or WinGetTitle($h) <> "DirectMusic Producer" Then Return False
    If Not _WinAPI_IsWindowVisible($h) Or Not _WinAPI_IsWindowEnabled($h) Then Return False
    If StringLower(_WinAPI_GetProcessFileName($gPid)) <> StringLower($gExe) Then Return False
    $body = Body($h)
    If $body <> "Failed to update the system registry." & @LF & "Please try using REGEDIT." And _
        $body <> "Unable to load DirectMusic Producer Components.  Please run Setup and reinstall." Then Return False
    $button = ControlGetHandle($h, "", "Button1")
    If Not $button Then Return False
    Local $buttonId = _WinAPI_GetDlgCtrlID($button)
    ; The observed registry-warning OK is ID 2, not the conventional IDOK 1.
    If $buttonId <> 1 And $buttonId <> 2 Then Return False
    Local $buttonPid = 0
    _WinAPI_GetWindowThreadProcessId($button, $buttonPid)
    Return $buttonPid = $gPid And _WinAPI_GetParent($button) = $h And _WinAPI_GetClassName($button) = "Button" And _
        ControlGetText($h, "", $button) = "OK" And _WinAPI_IsWindowVisible($button) And _WinAPI_IsWindowEnabled($button)
EndFunc
Func RectJson($r)
    Return '{"left":' & DllStructGetData($r,"Left") & ',"top":' & DllStructGetData($r,"Top") & _
        ',"right":' & DllStructGetData($r,"Right") & ',"bottom":' & DllStructGetData($r,"Bottom") & '}'
EndFunc

Local $path = _WinAPI_GetProcessFileName($gPid)
LogLine('{"operation":"process_identity","pid":' & $gPid & ',"path":' & J($path) & '}')
If StringLower($path) <> StringLower($gExe) Then Fail("Process path mismatch")
Local $windows = WinList(), $matches = 0, $dialog = 0, $button = 0, $body = ""
For $i = 1 To $windows[0][0]
    If WinGetProcess($windows[$i][1]) <> $gPid Then ContinueLoop
    Local $candidate = $windows[$i][1], $candidateButton = ControlGetHandle($candidate, "", "Button1")
    LogLine('{"operation":"owned_window","handle":' & Number($candidate) & ',"title":' & J(WinGetTitle($candidate)) & _
        ',"class":' & J(_WinAPI_GetClassName($candidate)) & ',"visible":' & StringLower(String(_WinAPI_IsWindowVisible($candidate))) & _
        ',"enabled":' & StringLower(String(_WinAPI_IsWindowEnabled($candidate))) & ',"classList":' & J(WinGetClassList($candidate)) & _
        ',"body":' & J(Body($candidate)) & ',"button":' & Number($candidateButton) & ',"buttonId":' & _WinAPI_GetDlgCtrlID($candidateButton) & ',"buttonText":' & J(ControlGetText($candidate,"",$candidateButton)) & '}')
    If Valid($windows[$i][1], $button, $body) Then
        $matches += 1
        $dialog = $windows[$i][1]
    EndIf
Next
LogLine('{"operation":"known_dialog_count","count":' & $matches & '}')
If $matches <> 1 Then Fail("Expected one known startup dialog")
If Not Valid($dialog, $button, $body) Then Fail("Dialog changed")
WinActivate($dialog)
If Not WinWaitActive($dialog, "", 3) Then Fail("Cannot foreground verified dialog for screenshot")
If Not Valid($dialog, $button, $body) Then Fail("Dialog changed before screenshot")
Local $dr = _WinAPI_GetWindowRect($dialog), $br = _WinAPI_GetWindowRect($button)
If Not IsDllStruct($dr) Or Not IsDllStruct($br) Then Fail("Cannot retrieve bounds")
Local $left = DllStructGetData($dr,"Left"), $top = DllStructGetData($dr,"Top")
Local $width = DllStructGetData($dr,"Right")-$left, $height = DllStructGetData($dr,"Bottom")-$top
If $width <= 0 Or $height <= 0 Or $width > 4096 Or $height > 4096 Then Fail("Invalid dialog bounds")
If Not _ScreenCapture_Capture($gPng, $left, $top, $left+$width-1, $top+$height-1, False) Then Fail("Screenshot failed")
LogLine('{"operation":"dialog_snapshot","pid":' & $gPid & ',"handle":' & Number($dialog) & ',"body":' & J($body) & _
    ',"screenshot":' & J($gPng) & ',"captured":true,"buttonId":' & _WinAPI_GetDlgCtrlID($button) & ',"dialogScreen":' & RectJson($dr) & ',"okScreen":' & RectJson($br) & _
    ',"okCenterInScreenshot":{"x":' & Int((DllStructGetData($br,"Left")+DllStructGetData($br,"Right"))/2-$left) & _
    ',"y":' & Int((DllStructGetData($br,"Top")+DllStructGetData($br,"Bottom"))/2-$top) & '}}')
If $gMode = "click" Then
    Local $verifiedButton = 0, $verifiedBody = ""
    If Not Valid($dialog, $verifiedButton, $verifiedBody) Or $verifiedButton <> $button Or $verifiedBody <> $body Then Fail("Dialog changed before click")
    Local $clicked = ControlClick($dialog, "", $button, "left", 1)
    Local $closed = WinWaitClose($dialog, "", 3)
    LogLine('{"operation":"dialog_ok","method":"AutoIt.ControlClick","result":' & $clicked & ',"dialogDismissed":' & StringLower(String($closed <> 0)) & '}')
    If $clicked <> 1 Or Not $closed Then
        FileClose($gLog)
        Exit 1
    EndIf
EndIf
FileClose($gLog)
Exit 0

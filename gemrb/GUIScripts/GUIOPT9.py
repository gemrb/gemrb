#autopause
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	GamePlayWindow = GemRB.LoadWindow(9)
	B1 = GemRB.GetControl(GamePlayWindow, 10)
	B2 = GemRB.GetControl(GamePlayWindow, 11)
	B3 = GemRB.GetControl(GamePlayWindow, 12)
	B4 = GemRB.GetControl(GamePlayWindow, 13)
	B5 = GemRB.GetControl(GamePlayWindow, 14)
	B6 = GemRB.GetControl(GamePlayWindow, 15)
	OkButton = GemRB.GetControl(GamePlayWindow, 26)
	CancelButton = GemRB.GetControl(GamePlayWindow, 27)
	TextAreaControl = GemRB.GetControl(GamePlayWindow, 28)

        GemRB.SetText(GamePlayWindow, OkButton, 11973)
        GemRB.SetText(GamePlayWindow, CancelButton, 13727)
	
	GemRB.SetEvent(GamePlayWindow, B1, 0x00000000, "B1Press")
	GemRB.SetEvent(GamePlayWindow, B2, 0x00000000, "B2Press")
	GemRB.SetEvent(GamePlayWindow, B3, 0x00000000, "B3Press")
	GemRB.SetEvent(GamePlayWindow, B4, 0x00000000, "B4Press")
	GemRB.SetEvent(GamePlayWindow, B5, 0x00000000, "B5Press")
	GemRB.SetEvent(GamePlayWindow, B6, 0x00000000, "B6Press")
        GemRB.SetEvent(GamePlayWindow, OkButton, 0x00000000, "OkPress")
        GemRB.SetEvent(GamePlayWindow, CancelButton, 0x00000000, "CancelPress")
        GemRB.ShowModal(GamePlayWindow)
	return

def B1Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18026)
	return

def B2Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18027)
	return

def B3Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18028)
	return

def B4Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18029)
	return

def B5Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18030)
	return

def B6Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18031)
	return

def OkPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetVisible(GamePlayWindow, 0)
        GemRB.UnloadWindow(GamePlayWindow)
        GemRB.SetNextScript("GUIOPT8")
        return

def CancelPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetVisible(GamePlayWindow, 0)
        GemRB.UnloadWindow(GamePlayWindow)
        GemRB.SetNextScript("GUIOPT8")
        return


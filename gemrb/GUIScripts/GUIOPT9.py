#autopause
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow
	GemRB.LoadWindowPack("GUIOPT")
	GamePlayWindow = GemRB.LoadWindow(9)
	B1 = GemRB.GetControl(GamePlayWindow, 0x40000000+15)
	B2 = GemRB.GetControl(GamePlayWindow, 0x40000000+16)
	B3 = GemRB.GetControl(GamePlayWindow, 0x40000000+17)
	B4 = GemRB.GetControl(GamePlayWindow, 0x40000000+18)
	B5 = GemRB.GetControl(GamePlayWindow, 0x40000000+19)
	B6 = GemRB.GetControl(GamePlayWindow, 0x40000000+20)
	B7 = GemRB.GetControl(GamePlayWindow, 0x40000000+21)
	B8 = GemRB.GetControl(GamePlayWindow, 0x40000000+22)
	B9 = GemRB.GetControl(GamePlayWindow, 0x40000000+23)
	B10 = GemRB.GetControl(GamePlayWindow, 0x40000000+24)
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
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
	return

def B2Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
	return

def B3Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
	return

def B4Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
	return

def B5Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
	return

def B6Press():
        global GamePlayWindow, TextAreaControl
	GemRB.SetText(GamePlayWindow, TextAreaControl, 18024)
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


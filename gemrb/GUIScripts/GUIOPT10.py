#autopause
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow
	GemRB.LoadWindowPack("GUIOPT")
	GamePlayWindow = GemRB.LoadWindow(10)
	OkButton = GemRB.GetControl(GamePlayWindow, 11)
	CancelButton = GemRB.GetControl(GamePlayWindow, 14)

        GemRB.SetText(GamePlayWindow, OkButton, 11973)
        GemRB.SetText(GamePlayWindow, CancelButton, 13727)

        GemRB.SetEvent(GamePlayWindow, OkButton, 0x00000000, "OkPress")
        GemRB.SetEvent(GamePlayWindow, CancelButton, 0x00000000, "CancelPress")
        GemRB.ShowModal(GamePlayWindow)
	return

def B1Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(OptionsWindow, TextAreaControl, 18026)
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


#autopause
import GemRB

GamePlayWindow = 0
TextAreaControl = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	GamePlayWindow = GemRB.LoadWindow(10)
	TextAreaControl = GemRB.GetControl(GamePlayWindow, 15)
	B1 = GemRB.GetControl(GamePlayWindow, 17)
	B1B = GemRB.GetControl(GamePlayWindow, 1)
	B2 = GemRB.GetControl(GamePlayWindow, 18)
	B2B = GemRB.GetControl(GamePlayWindow, 2)
	B3 = GemRB.GetControl(GamePlayWindow, 19)
	B3B = GemRB.GetControl(GamePlayWindow, 3)
	B4 = GemRB.GetControl(GamePlayWindow, 20)
	B4B = GemRB.GetControl(GamePlayWindow, 4)
	B5 = GemRB.GetControl(GamePlayWindow, 21)
	B5B = GemRB.GetControl(GamePlayWindow, 5)
	B6 = GemRB.GetControl(GamePlayWindow, 22)
	B6B = GemRB.GetControl(GamePlayWindow, 13)
	B7 = GemRB.GetControl(GamePlayWindow, 24)
	B7B = GemRB.GetControl(GamePlayWindow, 25)
	B8 = GemRB.GetControl(GamePlayWindow, 27)
	B8B = GemRB.GetControl(GamePlayWindow, 26)
	B9 = GemRB.GetControl(GamePlayWindow, 30)
	B9B = GemRB.GetControl(GamePlayWindow, 34)
	B10 = GemRB.GetControl(GamePlayWindow, 33)
	B10B = GemRB.GetControl(GamePlayWindow, 31)
	B11 = GemRB.GetControl(GamePlayWindow, 36)
	B11B = GemRB.GetControl(GamePlayWindow, 37)

	OkButton = GemRB.GetControl(GamePlayWindow, 11)
	CancelButton = GemRB.GetControl(GamePlayWindow, 14)
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18044)
        GemRB.SetText(GamePlayWindow, OkButton, 11973)
        GemRB.SetText(GamePlayWindow, CancelButton, 13727)

	GemRB.SetEvent(GamePlayWindow, B1, 0x00000000, "B1Press")
	GemRB.SetEvent(GamePlayWindow, B1B, 0x00000000, "B1BPress")

	GemRB.SetEvent(GamePlayWindow, B2, 0x00000000, "B2Press")
	GemRB.SetEvent(GamePlayWindow, B2B, 0x00000000, "B2BPress")

	GemRB.SetEvent(GamePlayWindow, B3, 0x00000000, "B3Press")
	GemRB.SetEvent(GamePlayWindow, B3B, 0x00000000, "B3BPress")

	GemRB.SetEvent(GamePlayWindow, B4, 0x00000000, "B4Press")
	GemRB.SetEvent(GamePlayWindow, B4B, 0x00000000, "B4BPress")

	GemRB.SetEvent(GamePlayWindow, B5, 0x00000000, "B5Press")
	GemRB.SetEvent(GamePlayWindow, B5B, 0x00000000, "B5BPress")

	GemRB.SetEvent(GamePlayWindow, B6, 0x00000000, "B6Press")
	GemRB.SetEvent(GamePlayWindow, B6B, 0x00000000, "B6BPress")

	GemRB.SetEvent(GamePlayWindow, B7, 0x00000000, "B7Press")
	GemRB.SetEvent(GamePlayWindow, B7B, 0x00000000, "B7BPress")

	GemRB.SetEvent(GamePlayWindow, B8, 0x00000000, "B8Press")
	GemRB.SetEvent(GamePlayWindow, B8B, 0x00000000, "B8BPress")

	GemRB.SetEvent(GamePlayWindow, B9, 0x00000000, "B9Press")
	GemRB.SetEvent(GamePlayWindow, B9B, 0x00000000, "B9BPress")

	GemRB.SetEvent(GamePlayWindow, B10, 0x00000000, "B10Press")
	GemRB.SetEvent(GamePlayWindow, B10B, 0x00000000, "B10BPress")

	GemRB.SetEvent(GamePlayWindow, B11, 0x00000000, "B11Press")
	GemRB.SetEvent(GamePlayWindow, B11B, 0x00000000, "B11BPress")

        GemRB.SetEvent(GamePlayWindow, OkButton, 0x00000000, "OkPress")
        GemRB.SetEvent(GamePlayWindow, CancelButton, 0x00000000, "CancelPress")
        GemRB.ShowModal(GamePlayWindow)
	return

def B1Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18032)
	return

def B1BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18032)
#also toggle character hit autopause variable
	return

def B2Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18033)
	return

def B2BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18033)
#also toggle character injured autopause
	return

def B3Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18034)
	return

def B3BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18034)
#also toggle character dead
	return

def B4Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18035)
	return

def B4BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18035)
#also toggle character attacked
	return

def B5Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18036)
	return

def B5BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18036)
#also toggle weapon unusable
	return

def B6Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18037)
	return

def B6BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18037)
#also toggle target destroyed
	return

def B7Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 10640)
	return

def B7BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 10640)
	return

def B8Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 23514)
	return

def B8BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 23514)
	return

def B9Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 58171)
	return

def B9BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 58171)
	return

def B10Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 31872)
	return

def B10BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 31872)
	return

def B11Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 10571)
	return

def B11BPress():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 10571)
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


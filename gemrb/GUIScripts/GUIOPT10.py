#autopause
import GemRB

GamePlayWindow = 0
TextAreaControl = 0
B1B = 0
B1BState = 0

def OnLoad():
	global GamePlayWindow, TextAreaControl, B1B, B1BState
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

	GemRB.SetEvent(GamePlayWindow, B1, IE_GUI_BUTTON_ON_PRESS, "B1Press")
	GemRB.SetEvent(GamePlayWindow, B1B, IE_GUI_BUTTON_ON_PRESS, "B1BPress")

	GemRB.SetEvent(GamePlayWindow, B2, IE_GUI_BUTTON_ON_PRESS, "B2Press")
	GemRB.SetEvent(GamePlayWindow, B2B, IE_GUI_BUTTON_ON_PRESS, "B2BPress")

	GemRB.SetEvent(GamePlayWindow, B3, IE_GUI_BUTTON_ON_PRESS, "B3Press")
	GemRB.SetEvent(GamePlayWindow, B3B, IE_GUI_BUTTON_ON_PRESS, "B3BPress")

	GemRB.SetEvent(GamePlayWindow, B4, IE_GUI_BUTTON_ON_PRESS, "B4Press")
	GemRB.SetEvent(GamePlayWindow, B4B, IE_GUI_BUTTON_ON_PRESS, "B4BPress")

	GemRB.SetEvent(GamePlayWindow, B5, IE_GUI_BUTTON_ON_PRESS, "B5Press")
	GemRB.SetEvent(GamePlayWindow, B5B, IE_GUI_BUTTON_ON_PRESS, "B5BPress")

	GemRB.SetEvent(GamePlayWindow, B6, IE_GUI_BUTTON_ON_PRESS, "B6Press")
	GemRB.SetEvent(GamePlayWindow, B6B, IE_GUI_BUTTON_ON_PRESS, "B6BPress")

	GemRB.SetEvent(GamePlayWindow, B7, IE_GUI_BUTTON_ON_PRESS, "B7Press")
	GemRB.SetEvent(GamePlayWindow, B7B, IE_GUI_BUTTON_ON_PRESS, "B7BPress")

	GemRB.SetEvent(GamePlayWindow, B8, IE_GUI_BUTTON_ON_PRESS, "B8Press")
	GemRB.SetEvent(GamePlayWindow, B8B, IE_GUI_BUTTON_ON_PRESS, "B8BPress")

	GemRB.SetEvent(GamePlayWindow, B9, IE_GUI_BUTTON_ON_PRESS, "B9Press")
	GemRB.SetEvent(GamePlayWindow, B9B, IE_GUI_BUTTON_ON_PRESS, "B9BPress")

	GemRB.SetEvent(GamePlayWindow, B10, IE_GUI_BUTTON_ON_PRESS, "B10Press")
	GemRB.SetEvent(GamePlayWindow, B10B, IE_GUI_BUTTON_ON_PRESS, "B10BPress")

	GemRB.SetEvent(GamePlayWindow, B11, IE_GUI_BUTTON_ON_PRESS, "B11Press")
	GemRB.SetEvent(GamePlayWindow, B11B, IE_GUI_BUTTON_ON_PRESS, "B11BPress")

        GemRB.SetEvent(GamePlayWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
        GemRB.SetEvent(GamePlayWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.ShowModal(GamePlayWindow)
	return

def B1Press():
        global GamePlayWindow, TextAreaControl
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18032)
	return

def B1BPress():
        global GamePlayWindow, TextAreaControl, B1B, B1BState
        GemRB.SetText(GamePlayWindow, TextAreaControl, 18032)
#also toggle character hit autopause variable
	if B1BState == IE_GUI_BUTTON_UNPRESSED:
		B1BState = IE_GUI_BUTTON_SELECTED
	else:
		B1BState = IE_GUI_BUTTON_UNPRESSED
	GemRB.SetButtonState(GamePlayWindow, B1B, B1BState)
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


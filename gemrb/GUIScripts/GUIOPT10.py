#autopause
import GemRB

AutoPauseWindow = 0
TextAreaControl = 0

def OnLoad():
	global AutoPauseWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	AutoPauseWindow = GemRB.LoadWindow(10)
	TextAreaControl = GemRB.GetControl(AutoPauseWindow, 15)
	B1 = GemRB.GetControl(AutoPauseWindow, 17)
	B1B = GemRB.GetControl(AutoPauseWindow, 1)
	B2 = GemRB.GetControl(AutoPauseWindow, 18)
	B2B = GemRB.GetControl(AutoPauseWindow, 2)
	B3 = GemRB.GetControl(AutoPauseWindow, 19)
	B3B = GemRB.GetControl(AutoPauseWindow, 3)
	B4 = GemRB.GetControl(AutoPauseWindow, 20)
	B4B = GemRB.GetControl(AutoPauseWindow, 4)
	B5 = GemRB.GetControl(AutoPauseWindow, 21)
	B5B = GemRB.GetControl(AutoPauseWindow, 5)
	B6 = GemRB.GetControl(AutoPauseWindow, 22)
	B6B = GemRB.GetControl(AutoPauseWindow, 13)
	B7 = GemRB.GetControl(AutoPauseWindow, 24)
	B7B = GemRB.GetControl(AutoPauseWindow, 25)
	B8 = GemRB.GetControl(AutoPauseWindow, 27)
	B8B = GemRB.GetControl(AutoPauseWindow, 26)
	B9 = GemRB.GetControl(AutoPauseWindow, 30)
	B9B = GemRB.GetControl(AutoPauseWindow, 34)
	B10 = GemRB.GetControl(AutoPauseWindow, 33)
	B10B = GemRB.GetControl(AutoPauseWindow, 31)
	B11 = GemRB.GetControl(AutoPauseWindow, 36)
	B11B = GemRB.GetControl(AutoPauseWindow, 37)

	OkButton = GemRB.GetControl(AutoPauseWindow, 11)
	CancelButton = GemRB.GetControl(AutoPauseWindow, 14)
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18044)
        GemRB.SetText(AutoPauseWindow, OkButton, 11973)
        GemRB.SetText(AutoPauseWindow, CancelButton, 13727)

	GemRB.SetEvent(AutoPauseWindow, B1, IE_GUI_BUTTON_ON_PRESS, "B1Press")
	GemRB.SetEvent(AutoPauseWindow, B1B, IE_GUI_BUTTON_ON_PRESS, "B1BPress")

	GemRB.SetEvent(AutoPauseWindow, B2, IE_GUI_BUTTON_ON_PRESS, "B2Press")
	GemRB.SetEvent(AutoPauseWindow, B2B, IE_GUI_BUTTON_ON_PRESS, "B2BPress")

	GemRB.SetEvent(AutoPauseWindow, B3, IE_GUI_BUTTON_ON_PRESS, "B3Press")
	GemRB.SetEvent(AutoPauseWindow, B3B, IE_GUI_BUTTON_ON_PRESS, "B3BPress")

	GemRB.SetEvent(AutoPauseWindow, B4, IE_GUI_BUTTON_ON_PRESS, "B4Press")
	GemRB.SetEvent(AutoPauseWindow, B4B, IE_GUI_BUTTON_ON_PRESS, "B4BPress")

	GemRB.SetEvent(AutoPauseWindow, B5, IE_GUI_BUTTON_ON_PRESS, "B5Press")
	GemRB.SetEvent(AutoPauseWindow, B5B, IE_GUI_BUTTON_ON_PRESS, "B5BPress")

	GemRB.SetEvent(AutoPauseWindow, B6, IE_GUI_BUTTON_ON_PRESS, "B6Press")
	GemRB.SetEvent(AutoPauseWindow, B6B, IE_GUI_BUTTON_ON_PRESS, "B6BPress")

	GemRB.SetEvent(AutoPauseWindow, B7, IE_GUI_BUTTON_ON_PRESS, "B7Press")
	GemRB.SetEvent(AutoPauseWindow, B7B, IE_GUI_BUTTON_ON_PRESS, "B7BPress")

	GemRB.SetEvent(AutoPauseWindow, B8, IE_GUI_BUTTON_ON_PRESS, "B8Press")
	GemRB.SetEvent(AutoPauseWindow, B8B, IE_GUI_BUTTON_ON_PRESS, "B8BPress")

	GemRB.SetEvent(AutoPauseWindow, B9, IE_GUI_BUTTON_ON_PRESS, "B9Press")
	GemRB.SetEvent(AutoPauseWindow, B9B, IE_GUI_BUTTON_ON_PRESS, "B9BPress")

	GemRB.SetEvent(AutoPauseWindow, B10, IE_GUI_BUTTON_ON_PRESS, "B10Press")
	GemRB.SetEvent(AutoPauseWindow, B10B, IE_GUI_BUTTON_ON_PRESS, "B10BPress")

	GemRB.SetEvent(AutoPauseWindow, B11, IE_GUI_BUTTON_ON_PRESS, "B11Press")
	GemRB.SetEvent(AutoPauseWindow, B11B, IE_GUI_BUTTON_ON_PRESS, "B11BPress")

        GemRB.SetEvent(AutoPauseWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
        GemRB.SetEvent(AutoPauseWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.ShowModal(AutoPauseWindow)
	return

def B1Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18032)
	return

def B1BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18032)
#also toggle character hit autopause variable
	return

def B2Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18033)
	return

def B2BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18033)
#also toggle character injured autopause
	return

def B3Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18034)
	return

def B3BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18034)
#also toggle character dead
	return

def B4Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18035)
	return

def B4BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18035)
#also toggle character attacked
	return

def B5Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18036)
	return

def B5BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18036)
#also toggle weapon unusable
	return

def B6Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18037)
	return

def B6BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 18037)
#also toggle target destroyed
	return

def B7Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10640)
	return

def B7BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10640)
	return

def B8Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 23514)
	return

def B8BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 23514)
	return

def B9Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 58171)
	return

def B9BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 58171)
	return

def B10Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 31872)
	return

def B10BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 31872)
	return

def B11Press():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10571)
	return

def B11BPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetText(AutoPauseWindow, TextAreaControl, 10571)
	return

def OkPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetVisible(AutoPauseWindow, 0)
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GUIOPT8")
        return

def CancelPress():
        global AutoPauseWindow, TextAreaControl
        GemRB.SetVisible(AutoPauseWindow, 0)
        GemRB.UnloadWindow(AutoPauseWindow)
        GemRB.SetNextScript("GUIOPT8")
        return


#feedback
import GemRB

FeedbackWindow = 0
TextAreaControl = 0

def OnLoad():
	global FeedbackWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	FeedbackWindow = GemRB.LoadWindow(9)

	S1 = GemRB.GetControl(FeedbackWindow, 30)
	S1S = GemRB.GetControl(FeedbackWindow, 8)

	S2 = GemRB.GetControl(FeedbackWindow, 31)
	S2S = GemRB.GetControl(FeedbackWindow, 9)

	B1 = GemRB.GetControl(FeedbackWindow, 32)
	B1B = GemRB.GetControl(FeedbackWindow, 10)

	B2 = GemRB.GetControl(FeedbackWindow, 33)
	B2B = GemRB.GetControl(FeedbackWindow, 11)

	B3 = GemRB.GetControl(FeedbackWindow, 34)
	B3B = GemRB.GetControl(FeedbackWindow, 12)

	B4 = GemRB.GetControl(FeedbackWindow, 35)
	B4B = GemRB.GetControl(FeedbackWindow, 13)

	B5 = GemRB.GetControl(FeedbackWindow, 36)
	B5B = GemRB.GetControl(FeedbackWindow, 14)

	B6 = GemRB.GetControl(FeedbackWindow, 37)
	B6B = GemRB.GetControl(FeedbackWindow, 15)
	OkButton = GemRB.GetControl(FeedbackWindow, 26)
	CancelButton = GemRB.GetControl(FeedbackWindow, 27)
	TextAreaControl = GemRB.GetControl(FeedbackWindow, 28)

        GemRB.SetText(FeedbackWindow, TextAreaControl, 18043)
        GemRB.SetText(FeedbackWindow, OkButton, 11973)
        GemRB.SetText(FeedbackWindow, CancelButton, 13727)
	
	GemRB.SetEvent(FeedbackWindow, S1, IE_GUI_BUTTON_ON_PRESS, "S1Press")
	GemRB.SetEvent(FeedbackWindow, S1S, IE_GUI_SLIDER_ON_CHANGE, "S1SChange")
	GemRB.SetEvent(FeedbackWindow, S2, IE_GUI_BUTTON_ON_PRESS, "S2Press")
	GemRB.SetEvent(FeedbackWindow, S2S, IE_GUI_SLIDER_ON_CHANGE, "S2SChange")

	GemRB.SetEvent(FeedbackWindow, B1, IE_GUI_BUTTON_ON_PRESS, "B1Press")
	GemRB.SetEvent(FeedbackWindow, B1B, IE_GUI_BUTTON_ON_PRESS, "B1BPress")
	GemRB.SetEvent(FeedbackWindow, B2, IE_GUI_BUTTON_ON_PRESS, "B2Press")
	GemRB.SetEvent(FeedbackWindow, B2B, IE_GUI_BUTTON_ON_PRESS, "B2BPress")
	GemRB.SetEvent(FeedbackWindow, B3, IE_GUI_BUTTON_ON_PRESS, "B3Press")
	GemRB.SetEvent(FeedbackWindow, B3B, IE_GUI_BUTTON_ON_PRESS, "B3BPress")
	GemRB.SetEvent(FeedbackWindow, B4, IE_GUI_BUTTON_ON_PRESS, "B4Press")
	GemRB.SetEvent(FeedbackWindow, B4B, IE_GUI_BUTTON_ON_PRESS, "B4BPress")
	GemRB.SetEvent(FeedbackWindow, B5, IE_GUI_BUTTON_ON_PRESS, "B5Press")
	GemRB.SetEvent(FeedbackWindow, B5B, IE_GUI_BUTTON_ON_PRESS, "B5BPress")
	GemRB.SetEvent(FeedbackWindow, B6, IE_GUI_BUTTON_ON_PRESS, "B6Press")
	GemRB.SetEvent(FeedbackWindow, B6B, IE_GUI_BUTTON_ON_PRESS, "B6BPress")
        GemRB.SetEvent(FeedbackWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
        GemRB.SetEvent(FeedbackWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.ShowModal(FeedbackWindow)
	return

def S1Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18024)
	return

def S1SChange():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18024)
#also handle slider1
	return

def S2Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18025)
	return

def S2SChange():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18025)
#also handle slider2
	return

def B1Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18026)
	return

def B1BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18026)
#also handle to hit feedback
	return

def B2Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18027)
	return

def B2BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18027)
#also handle combat info
	return

def B3Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18028)
	return

def B3BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18028)
#also handle actions
	return

def B4Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18029)
	return

def B4BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18029)
#also handle state changes
	return

def B5Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18030)
	return

def B5BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18030)
#also handle selection
	return

def B6Press():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18031)
	return

def B6BPress():
        global FeedbackWindow, TextAreaControl
	GemRB.SetText(FeedbackWindow, TextAreaControl, 18031)
#also handle misc. feedback
	return

def OkPress():
        global FeedbackWindow, TextAreaControl
        GemRB.SetVisible(FeedbackWindow, 0)
        GemRB.UnloadWindow(FeedbackWindow)
        GemRB.SetNextScript("GUIOPT8")
        return

def CancelPress():
        global FeedbackWindow, TextAreaControl
        GemRB.SetVisible(FeedbackWindow, 0)
        GemRB.UnloadWindow(FeedbackWindow)
        GemRB.SetNextScript("GUIOPT8")
        return


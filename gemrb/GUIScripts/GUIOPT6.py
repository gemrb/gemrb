import GemRB

OptionsWindow = 0
TextAreaControl = 0

def OnLoad():
	global OptionsWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	OptionsWindow = GemRB.LoadWindow(6)
	TextAreaControl = GemRB.GetControl(OptionsWindow, 33)
	BrightnessButton = GemRB.GetControl(OptionsWindow, 35)
	ContrastButton = GemRB.GetControl(OptionsWindow, 36)
	BppButton = GemRB.GetControl(OptionsWindow, 37)
	FullScreenButton = GemRB.GetControl(OptionsWindow, 38)
	TransShadowButton = GemRB.GetControl(OptionsWindow, 50)
	SoftMirrBltButton = GemRB.GetControl(OptionsWindow, 44)
	SoftTransBltButton = GemRB.GetControl(OptionsWindow, 46)
	SoftStandBltButton = GemRB.GetControl(OptionsWindow, 48)
	OkButton = GemRB.GetControl(OptionsWindow, 21)
	CancelButton = GemRB.GetControl(OptionsWindow, 32)
	GemRB.SetText(OptionsWindow, TextAreaControl, 18038)
	GemRB.SetText(OptionsWindow, OkButton, 11973)
	GemRB.SetText(OptionsWindow, CancelButton, 13727)
	GemRB.SetEvent(OptionsWindow, BrightnessButton, IE_GUI_BUTTON_ON_PRESS, "BrightnessPress")
	GemRB.SetEvent(OptionsWindow, ContrastButton, IE_GUI_BUTTON_ON_PRESS, "ContrastPress")
	GemRB.SetEvent(OptionsWindow, BppButton, IE_GUI_BUTTON_ON_PRESS, "BppPress")
	GemRB.SetEvent(OptionsWindow, FullScreenButton, IE_GUI_BUTTON_ON_PRESS, "FullScreenPress")
	GemRB.SetEvent(OptionsWindow, TransShadowButton, IE_GUI_BUTTON_ON_PRESS, "TransShadowPress")
	GemRB.SetEvent(OptionsWindow, SoftMirrBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftMirrBltPress")
	GemRB.SetEvent(OptionsWindow, SoftTransBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftTransBltPress")
	GemRB.SetEvent(OptionsWindow, SoftStandBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftStandBltPress")
	GemRB.SetEvent(OptionsWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(OptionsWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(OptionsWindow)
	return
	
def BrightnessPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 17203)
	return
	
def ContrastPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 17204)
	return
	
def BppPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 17205)
	return
	
def FullScreenPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 18000)
	return
	
def TransShadowPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 20620)
	return
	
def SoftMirrBltPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 18004)
	return
	
def SoftTransBltPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 18006)
	return
	
def SoftStandBltPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetText(OptionsWindow, TextAreaControl, 18007)
	return
	
def OkPress():
	global OptionsWindow, TextAreaControl
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	global OptionsWindow, TextAreaControl
	GemRB.UnloadWindow(OptionsWindow)
	GemRB.SetNextScript("StartOpt")
	return

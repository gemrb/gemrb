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
	GemRB.SetText(OptionsWindow, CancelButton, 13957)
	GemRB.SetEvent(OptionsWindow, BrightnessButton, 0x00000000, "BrightnessPress")
	GemRB.SetEvent(OptionsWindow, ContrastButton, 0x00000000, "ContrastPress")
	GemRB.SetEvent(OptionsWindow, BppButton, 0x00000000, "BppPress")
	GemRB.SetEvent(OptionsWindow, FullScreenButton, 0x00000000, "FullScreenPress")
	GemRB.SetEvent(OptionsWindow, TransShadowButton, 0x00000000, "TransShadowPress")
	GemRB.SetEvent(OptionsWindow, SoftMirrBltButton, 0x00000000, "SoftMirrBltPress")
	GemRB.SetEvent(OptionsWindow, SoftTransBltButton, 0x00000000, "SoftTransBltPress")
	GemRB.SetEvent(OptionsWindow, SoftStandBltButton, 0x00000000, "SoftStandBltPress")
	GemRB.SetEvent(OptionsWindow, OkButton, 0x00000000, "OkPress")
	GemRB.SetEvent(OptionsWindow, CancelButton, 0x00000000, "CancelPress")
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
	GemRB.SetVisible(OptionsWindow, 0)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	global OptionsWindow, TextAreaControl
	GemRB.SetVisible(OptionsWindow, 0)
	GemRB.SetNextScript("StartOpt")
	return
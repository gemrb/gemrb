#graphics options
import GemRB

GraphicsWindow = 0
TextAreaControl = 0

def OnLoad():
	global GraphicsWindow, TextAreaControl
	GemRB.LoadWindowPack("GUIOPT")
	GraphicsWindow = GemRB.LoadWindow(6)
	TextAreaControl = GemRB.GetControl(GraphicsWindow, 33)
	BrightnessButton = GemRB.GetControl(GraphicsWindow, 35)
	BrightnessSlider = GemRB.GetControl(GraphicsWindow, 3)
	ContrastButton = GemRB.GetControl(GraphicsWindow, 36)
	ContrastSlider = GemRB.GetControl(GraphicsWindow, 22)
	BppButton = GemRB.GetControl(GraphicsWindow, 37)
	BppButtonB1 = GemRB.GetControl(GraphicsWindow, 5)
	BppButtonB2 = GemRB.GetControl(GraphicsWindow, 6)
	BppButtonB3 = GemRB.GetControl(GraphicsWindow, 7)
	GemRB.SetVarAssoc(GraphicsWindow, BppButtonB1, "BPP",16)
	GemRB.SetVarAssoc(GraphicsWindow, BppButtonB2, "BPP",24)
	GemRB.SetVarAssoc(GraphicsWindow, BppButtonB3, "BPP",32)
	GemRB.SetButtonFlags(GraphicsWindow, BppButtonB1, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetButtonFlags(GraphicsWindow, BppButtonB2, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetButtonFlags(GraphicsWindow, BppButtonB3, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FullScreenButton = GemRB.GetControl(GraphicsWindow, 38)
	FullScreenButtonB = GemRB.GetControl(GraphicsWindow, 9)
	SoftMirrBltButton = GemRB.GetControl(GraphicsWindow, 44)
	SoftMirrBltButtonB = GemRB.GetControl(GraphicsWindow, 40)
	SoftTransBltButton = GemRB.GetControl(GraphicsWindow, 46)
	SoftTransBltButtonB = GemRB.GetControl(GraphicsWindow, 41)
	SoftStandBltButton = GemRB.GetControl(GraphicsWindow, 48)
	SoftStandBltButtonB = GemRB.GetControl(GraphicsWindow, 42)
	TransShadowButton = GemRB.GetControl(GraphicsWindow, 50)
	TransShadowButtonB = GemRB.GetControl(GraphicsWindow, 51)
	OkButton = GemRB.GetControl(GraphicsWindow, 21)
	CancelButton = GemRB.GetControl(GraphicsWindow, 32)
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18038)
	GemRB.SetText(GraphicsWindow, OkButton, 11973)
	GemRB.SetText(GraphicsWindow, CancelButton, 13727)
	GemRB.SetEvent(GraphicsWindow, BrightnessButton, IE_GUI_BUTTON_ON_PRESS, "BrightnessPress")
	GemRB.SetEvent(GraphicsWindow, BrightnessSlider, IE_GUI_SLIDER_ON_CHANGE, "BrightnessPressB")
	GemRB.SetEvent(GraphicsWindow, ContrastButton, IE_GUI_BUTTON_ON_PRESS, "ContrastPress")
	GemRB.SetEvent(GraphicsWindow, ContrastSlider, IE_GUI_SLIDER_ON_CHANGE, "ContrastPressB")
	GemRB.SetEvent(GraphicsWindow, BppButton, IE_GUI_BUTTON_ON_PRESS, "BppPress")
	GemRB.SetEvent(GraphicsWindow, BppButtonB1, IE_GUI_BUTTON_ON_PRESS, "BppPressB1")
	GemRB.SetEvent(GraphicsWindow, BppButtonB2, IE_GUI_BUTTON_ON_PRESS, "BppPressB2")
	GemRB.SetEvent(GraphicsWindow, BppButtonB3, IE_GUI_BUTTON_ON_PRESS, "BppPressB3")
	GemRB.SetEvent(GraphicsWindow, FullScreenButton, IE_GUI_BUTTON_ON_PRESS, "FullScreenPress")
	GemRB.SetEvent(GraphicsWindow, FullScreenButtonB, IE_GUI_BUTTON_ON_PRESS, "FullScreenPressB")
	GemRB.SetEvent(GraphicsWindow, TransShadowButton, IE_GUI_BUTTON_ON_PRESS, "TransShadowPress")
	GemRB.SetEvent(GraphicsWindow, TransShadowButtonB, IE_GUI_BUTTON_ON_PRESS, "TransShadowPressB")
	GemRB.SetEvent(GraphicsWindow, SoftMirrBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftMirrBltPress")
	GemRB.SetEvent(GraphicsWindow, SoftMirrBltButtonB, IE_GUI_BUTTON_ON_PRESS, "SoftMirrBltPressB")
	GemRB.SetEvent(GraphicsWindow, SoftTransBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftTransBltPress")
	GemRB.SetEvent(GraphicsWindow, SoftTransBltButtonB, IE_GUI_BUTTON_ON_PRESS, "SoftTransBltPressB")
	GemRB.SetEvent(GraphicsWindow, SoftStandBltButton, IE_GUI_BUTTON_ON_PRESS, "SoftStandBltPress")
	GemRB.SetEvent(GraphicsWindow, SoftStandBltButtonB, IE_GUI_BUTTON_ON_PRESS, "SoftStandBltPressB")
	GemRB.SetEvent(GraphicsWindow, OkButton, IE_GUI_BUTTON_ON_PRESS, "OkPress")
	GemRB.SetEvent(GraphicsWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(GraphicsWindow)
	return
	
def BrightnessPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17203)
	return
	
def BrightnessPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17203)
#retrieve brightness slider state
	return
	
def ContrastPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17204)
	return
	
def ContrastPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17204)
#retrieve contrast slider state
	return
	
def BppPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17205)
	return
	
def BppPressB1():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17205)
#16 bpp
	return
	
def BppPressB2():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17205)
#24 bpp
	return
	
def BppPressB3():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 17205)
#32 bpp
	return
	
def FullScreenPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18000)
	return
	
def FullScreenPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18000)
#retrieve
	return
	
def TransShadowPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 20620)
	return
	
def TransShadowPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 20620)
#retrieve
	return
	
def SoftMirrBltPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18004)
	return
	
def SoftMirrBltPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18004)
#retrieve
	return
	
def SoftTransBltPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18006)
	return
	
def SoftTransBltPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18006)
#retrieve
	return
	
def SoftStandBltPress():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18007)
	return
	
def SoftStandBltPressB():
	GemRB.SetText(GraphicsWindow, TextAreaControl, 18007)
#retrieve
	return
	
def OkPress():
	GemRB.UnloadWindow(GraphicsWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	GemRB.UnloadWindow(GraphicsWindow)
	GemRB.SetNextScript("StartOpt")
	return

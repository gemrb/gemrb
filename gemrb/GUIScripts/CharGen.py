#character generation (GUICG 0)
import GemRB

CharGenWindow = 0

def OnLoad():
	global CharGenWindow

	GemRB.LoadWindowPack("GUICG")
        CharGenWindow = GemRB.LoadWindow(0)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

        CancelButton = GemRB.GetControl(CharGenWindow, 15)
#        NextButton = GemRB.GetControl(CharGenWindow, 10)
        GemRB.SetText(CharGenWindow, CancelButton, 13727)
#        GemRB.SetText(CharGenWindow, NextButton, 33093)
#        GemRB.SetEvent(CharGenWindow, NextButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
        GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
#	GemRB.ShowModal(CharGenWindow)
	GemRB.SetVisible(CharGenWindow,1)
	return
	
def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG1") #gender
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.SetNextScript("Start")
        return

#character generation (GUICG 0)
import GemRB

CharGenWindow = 0

def OnLoad():
	global CharGenWindow

	GemRB.LoadWindowPack("GUICG")
        CharGenWindow = GemRB.LoadWindow(0)
#        CancelButton = GemRB.GetControl(CharGenWindow, 11)
#        NextButton = GemRB.GetControl(CharGenWindow, 10)
#        GemRB.SetText(CharGenWindow, CancelButton, 13727)
#        GemRB.SetText(CharGenWindow, NextButton, 33093)
#        GemRB.SetEvent(CharGenWindow, NextButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
#        GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.ShowModal(CharGenWindow)
	return
	
def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG1") #gender
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.SetNextScript("Start")
        return

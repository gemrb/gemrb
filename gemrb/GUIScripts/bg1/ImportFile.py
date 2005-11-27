#character generation, import (GUICG24)
import GemRB

#import from a character sheet
ImportWindow = 0

def OnLoad():
	global ImportWindow

	GemRB.LoadWindowPack("GUICG")
	ImportWindow = GemRB.LoadWindow(20)

	TextAreaControl = GemRB.GetControl(ImportWindow, 4)
	GemRB.SetText(ImportWindow, TextAreaControl, 10963)

	TextAreaControl = GemRB.GetControl(ImportWindow,2)
#Fill TextArea Control with character sheets, make textarea a listbox

	DoneButton = GemRB.GetControl(ImportWindow, 0)
	GemRB.SetText(ImportWindow, DoneButton, 2610)
	GemRB.SetButtonState(ImportWindow, DoneButton, IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(ImportWindow,1)
	GemRB.SetText(ImportWindow, CancelButton, 15416)

	GemRB.SetEvent(ImportWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetEvent(ImportWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetVisible(ImportWindow,1)
	return

def DonePress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("Start")
	return
	
def CancelPress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("CharGen")
	return

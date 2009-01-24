#character generation, import (GUICG20)
import GemRB

#import from a character sheet
ImportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ImportWindow, TextAreaControl

	GemRB.LoadWindowPack("GUICG")
	ImportWindow = GemRB.LoadWindow(20)

	TextAreaControl = GemRB.GetControl(ImportWindow, 4)
	GemRB.SetText(ImportWindow, TextAreaControl, 10963)

	TextAreaControl = GemRB.GetControl(ImportWindow,2)
	GemRB.SetTextAreaFlags (ImportWindow, TextAreaControl, IE_GUI_TEXTAREA_SELECTABLE)
	GemRB.GetCharacters(ImportWindow, TextAreaControl)

	DoneButton = GemRB.GetControl(ImportWindow, 0)
	GemRB.SetText(ImportWindow, DoneButton, 13955)
	GemRB.SetButtonState(ImportWindow, DoneButton, IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(ImportWindow,1)
	GemRB.SetText(ImportWindow, CancelButton, 15416)

	GemRB.SetEvent(ImportWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetEvent(ImportWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(ImportWindow, TextAreaControl, IE_GUI_TEXTAREA_ON_CHANGE, "SelectPress")
	GemRB.SetVisible(ImportWindow,1)
	return

def SelectPress():
	DoneButton = GemRB.GetControl(ImportWindow, 0)
	GemRB.SetButtonState(ImportWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def DonePress():
	FileName = GemRB.QueryText(ImportWindow, TextAreaControl)
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	GemRB.UnloadWindow(ImportWindow)
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return

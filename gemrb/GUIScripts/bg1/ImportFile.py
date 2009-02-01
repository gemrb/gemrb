#character generation, import (GUICG20)
import GemRB

#import from a character sheet
ImportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ImportWindow, TextAreaControl

	GemRB.LoadWindowPack("GUICG")
	ImportWindow = GemRB.LoadWindowObject(20)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(10963)

	TextAreaControl = ImportWindow.GetControl(2)
	TextAreaControl.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	TextAreaControl.GetCharacters()

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(13955)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DonePress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	TextAreaControl.SetEvent(IE_GUI_TEXTAREA_ON_CHANGE, "SelectPress")
	ImportWindow.SetVisible(1)
	return

def SelectPress():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def DonePress():
	FileName = TextAreaControl.QueryText()
	Slot = GemRB.GetVar("Slot")
	GemRB.CreatePlayer(FileName, Slot| 0x8000, 1)
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("CharGen7")
	return
	
def CancelPress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript(GemRB.GetToken("NextScript"))
	return

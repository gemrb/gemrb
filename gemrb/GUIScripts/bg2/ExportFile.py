#character generation, import (GUICG24)
import GemRB

#import from a character sheet
ImportWindow = 0
TextAreaControl = 0

def OnLoad():
	global ImportWindow, TextAreaControl

	GemRB.LoadWindowPack ("GUICG",640,480)
	ImportWindow = GemRB.LoadWindowObject (21)

	TextAreaControl = ImportWindow.GetControl (4)
	TextAreaControl.SetText (10962)

	TextAreaControl = ImportWindow.GetControl (2)
	TextAreaControl.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	TextAreaControl.GetCharacters ()
 
	DoneButton = ImportWindow.GetControl (0)
	DoneButton.SetText (2610)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl (1)
	CancelButton.SetText (15416)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DonePress")
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	TextAreaControl.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, "SelectPress")
	ImportWindow.SetVisible (1)
	return

def SelectPress ():
	DoneButton = ImportWindow.GetControl (0)
	DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DonePress ():
	FileName = TextAreaControl.QueryText ()
	Slot = GemRB.GetVar ("Slot")
	GemRB.SaveCharacter (Slot, FileName)
	if ImportWindow:
		ImportWindow.Unload ()
	GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return
	
def CancelPress ():
	if ImportWindow:
		ImportWindow.Unload ()
	GemRB.SetNextScript (GemRB.GetToken("NextScript"))
	return 

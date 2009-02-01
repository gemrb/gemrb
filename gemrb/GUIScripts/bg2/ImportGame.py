#character generation, import  (GUICG24)
import GemRB

#import from a game 
ImportWindow = 0

def OnLoad():
	global ImportWindow

	GemRB.LoadWindowPack("GUICG",640,480)
	ImportWindow = GemRB.LoadWindowObject(20)

	TextAreaControl = ImportWindow.GetControl(4)
	TextAreaControl.SetText(53774)

	TextAreaControl = ImportWindow.GetControl(2)
#Fill TextArea Control with character sheets, make textarea a listbox

	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(2610)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	CancelButton = ImportWindow.GetControl(1)
	CancelButton.SetText(15416)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "Done1Press")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	ImportWindow.SetVisible(1)
	return

def Done1Press():
	DoneButton = ImportWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "Done2Press")
	return
	
def Done2Press():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("Start")
	return

def CancelPress():
	if ImportWindow:
		ImportWindow.Unload()
	GemRB.SetNextScript("CharGen")
	return

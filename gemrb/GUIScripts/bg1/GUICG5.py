#character generation, name (GUICG5)
import GemRB

NameWindow = 0
NameField = 0
DoneButton = 0

def OnLoad():
	global NameWindow, NameField, DoneButton
	
	GemRB.LoadWindowPack("GUICG")
	NameWindow = GemRB.LoadWindowObject(5)

	BackButton = NameWindow.GetControl(3)
	BackButton.SetText(15416)

	DoneButton = NameWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	NameField = NameWindow.GetControl(2)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	NameField.SetEvent(IE_GUI_EDIT_ON_CHANGE,"EditChange")
	NameWindow.SetVisible(1)
	NameField.SetStatus(IE_GUI_CONTROL_FOCUSED)
	return

def BackPress():
	if NameWindow:
		NameWindow.Unload()
	GemRB.SetNextScript("CharGen8")
	return

def NextPress():
	Name = NameField.QueryText()
	#check length?
	#seems like a good idea to store it here for the time being
	GemRB.SetToken("CHARNAME",Name) 
	if NameWindow:
		NameWindow.Unload()
	GemRB.SetNextScript("CharGen9")
	return

def EditChange():
	Name = NameField.QueryText()
	if len(Name)==0:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

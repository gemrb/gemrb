#character generation, name (GUICG5)
import GemRB

NameWindow = 0
NameField = 0

def OnLoad():
	global NameWindow, NameField
	
	GemRB.LoadWindowPack("GUICG")
	NameWindow = GemRB.LoadWindow(5)

	BackButton = GemRB.GetControl(NameWindow,3)
	GemRB.SetText(NameWindow, BackButton, 15416)

	DoneButton = GemRB.GetControl(NameWindow,0)
	GemRB.SetText(NameWindow, DoneButton, 11973)

	NameField = GemRB.GetControl(NameWindow, 2)

	GemRB.SetEvent(NameWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(NameWindow, BackButton, IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(NameWindow,1)
	return

def BackPress():
	GemRB.UnloadWindow(NameWindow)
	GemRB.SetNextScript("CharGen8")
	return

def NextPress():
	Name = GemRB.QueryText(NameWindow, NameField)
	#check length?
	#seems like a good idea to store it here for the time being
	GemRB.SetToken("CHARNAME",Name) 
        GemRB.UnloadWindow(NameWindow)
	GemRB.SetNextScript("CharGen9")
	return


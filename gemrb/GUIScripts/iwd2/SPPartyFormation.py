#Single Player Party Formation
import GemRB

PartyFormationWindow = 0
ExitWindow = 0

def OnLoad():
	global PartyFormationWindow
	GemRB.LoadWindowPack("GUISP")
	
	PartyFormationWindow = GemRB.LoadWindow(0)
	
	ExitButton = GemRB.GetControl(PartyFormationWindow, 30)
	GemRB.SetText(PartyFormationWindow, ExitButton, 13906)
	GemRB.SetEvent(PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	ModifyCharactersButton = GemRB.GetControl(PartyFormationWindow, 43)
	GemRB.SetText(PartyFormationWindow, ModifyCharactersButton, 18816)
	GemRB.SetButtonState(PartyFormationWindow, ModifyCharactersButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(PartyFormationWindow,ModifyCharactersButton, IE_GUI_BUTTON_ON_PRESS,"ModifyCharactersPress")

	DoneButton = GemRB.GetControl(PartyFormationWindow, 28)
	GemRB.SetText(PartyFormationWindow, DoneButton, 11973)
	if GemRB.GetPartySize()==0:
		GemRB.SetButtonState(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PartyFormationWindow,ModifyCharactersButton, IE_GUI_BUTTON_ON_PRESS,"EnterGamePress")
	
	for i in range(18,24):
		Button = GemRB.GetControl(PartyFormationWindow,i)
		GemRB.SetVarAssoc(PartyFormationWindow, Button, "Slot",i-18)
		GemRB.SetText(PartyFormationWindow, Button, 10264)
		GemRB.SetEvent(PartyFormationWindow, Button, IE_GUI_BUTTON_ON_PRESS, "GeneratePress")
	
	GemRB.SetVisible(PartyFormationWindow, 1)
	
	return
	
def ExitPress():
	global PartyFormationWindow, ExitWindow
	GemRB.SetVisible(PartyFormationWindow, 0)
	ExitWindow = GemRB.LoadWindow(7)
	
	TextArea = GemRB.GetControl(ExitWindow, 0)
	GemRB.SetText(ExitWindow, TextArea, 11329)
	
	CancelButton = GemRB.GetControl(ExitWindow, 2)
	GemRB.SetText(ExitWindow, CancelButton, 13727)
	GemRB.SetEvent(ExitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExitCancelPress")
	
	DoneButton = GemRB.GetControl(ExitWindow, 1)
	GemRB.SetText(ExitWindow, DoneButton, 11973)
	GemRB.SetEvent(ExitWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "ExitDonePress")
	
	GemRB.SetVisible(ExitWindow, 1)
	return
	
def ExitDonePress():
	global PartyFormationWindow, ExitWindow
	GemRB.UnloadWindow(ExitWindow);
	GemRB.UnloadWindow(PartyFormationWindow)
	GemRB.SetNextScript("Start")
	return
	
def ExitCancelPress():
	global PartyFormationWindow, ExitWindow
	GemRB.UnloadWindow(ExitWindow)
	GemRB.SetVisible(PartyFormationWindow, 1)
	return
	
def GeneratePress():
	global PartyFormationWindow
	print "SLOT:", GemRB.GetVar("Slot")
	GemRB.UnloadWindow(PartyFormationWindow)
	GemRB.SetNextScript("CharGen")
	return

def EnterGamePress():
	GemRB.EnterGame()
	return


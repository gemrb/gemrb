#Single Player Party Formation
import GemRB

PartyFormationWindow = 0
ExitWindow = 0

def OnLoad():
	global PartyFormationWindow
	GemRB.LoadWindowPack("GUISP", 800, 600)
	
	PartyFormationWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame( PartyFormationWindow)
	ExitButton = GemRB.GetControl(PartyFormationWindow, 30)
	GemRB.SetText(PartyFormationWindow, ExitButton, 13906)
	GemRB.SetEvent(PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	ModifyCharactersButton = GemRB.GetControl(PartyFormationWindow, 43)
	GemRB.SetText(PartyFormationWindow, ModifyCharactersButton, 18816)
	GemRB.SetButtonState(PartyFormationWindow, ModifyCharactersButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(PartyFormationWindow,ModifyCharactersButton, IE_GUI_BUTTON_ON_PRESS,"ModifyCharactersPress")

	DoneButton = GemRB.GetControl(PartyFormationWindow, 28)
	GemRB.SetText(PartyFormationWindow, DoneButton, 11973)
	Portraits = 0
	
	for i in range(18,24):
		Label = GemRB.GetControl(PartyFormationWindow, 0x10000012+i)
		#removing this label, it just disturbs us
		GemRB.SetControlSize(PartyFormationWindow, Label, 0, 0)
		Button = GemRB.GetControl(PartyFormationWindow, i-12)
		ResRef = GemRB.GetPlayerPortrait(i-17, 1)
		if ResRef == "":
			GemRB.SetButtonFlags(PartyFormationWindow, Button, IE_GUI_BUTTON_NORMAL,OP_SET)
		else:
			GemRB.SetButtonPicture(PartyFormationWindow, Button, ResRef)
			GemRB.SetButtonFlags(PartyFormationWindow, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			Portraits = Portraits+1

		GemRB.SetVarAssoc(PartyFormationWindow, Button, "Slot",i-17)
		GemRB.SetEvent(PartyFormationWindow, Button, IE_GUI_BUTTON_ON_PRESS, "GeneratePress")

		Button = GemRB.GetControl(PartyFormationWindow, i)
		GemRB.SetVarAssoc(PartyFormationWindow, Button, "Slot",i-17)
		if ResRef == "":
			GemRB.SetText(PartyFormationWindow, Button, 10264)
		else:
			GemRB.SetText(PartyFormationWindow, Button, GemRB.GetPlayerName(i-17,0) )

		GemRB.SetEvent(PartyFormationWindow, Button, IE_GUI_BUTTON_ON_PRESS, "GeneratePress")
	
	if Portraits == 0:
		GemRB.SetButtonState(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonFlags(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DEFAULT, OP_OR)
	GemRB.SetEvent(PartyFormationWindow,DoneButton, IE_GUI_BUTTON_ON_PRESS,"EnterGamePress")

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
	GemRB.UnloadWindow(ExitWindow)
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
	slot = GemRB.GetVar("Slot")
	ResRef = GemRB.GetPlayerPortrait(slot, 0)
	if ResRef:
		print "Already existing slot, we should drop it"
	GemRB.UnloadWindow(PartyFormationWindow)
	GemRB.SetNextScript("CharGen")
	return

def EnterGamePress():
	GemRB.SetVar("PlayMode",2)   #iwd2 is always using 'mpsave'
	GemRB.EnterGame()
	return


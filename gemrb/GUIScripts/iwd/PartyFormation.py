#Single Player Party Formation
import GemRB

PartyFormationWindow = 0
CreateCharWindow = 0
ExitWindow = 0

def OnLoad():
	global PartyFormationWindow
	GemRB.LoadWindowPack("GUISP")
	PartyFormationWindow = GemRB.LoadWindow(0)

	ModifyCharsButton = GemRB.GetControl(PartyFormationWindow, 43)
	GemRB.SetEvent(PartyFormationWindow, ModifyCharsButton, IE_GUI_BUTTON_ON_PRESS, "ModifyPress")
	GemRB.SetControlStatus(PartyFormationWindow, ModifyCharsButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText(PartyFormationWindow, ModifyCharsButton, 18816)
	
	DoneButton = GemRB.GetControl(PartyFormationWindow, 28)
	GemRB.SetEvent(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetControlStatus(PartyFormationWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText(PartyFormationWindow, DoneButton, 11973)
	
	ExitButton = GemRB.GetControl(PartyFormationWindow, 30)
	GemRB.SetEvent(PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetControlStatus(PartyFormationWindow, ExitButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(PartyFormationWindow, ExitButton, 13906)

	for i in range(18,24):
		Label = GemRB.GetControl(PartyFormationWindow, 0x10000012+i)
		#removing this label, it just disturbs us
		GemRB.SetControlSize(PartyFormationWindow, Label, 0, 0)

		CreateCharButton = GemRB.GetControl(PartyFormationWindow,i)
		GemRB.SetEvent(PartyFormationWindow, CreateCharButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharPress")
		GemRB.SetControlStatus(PartyFormationWindow, CreateCharButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetText(PartyFormationWindow, CreateCharButton, 10264)

	GemRB.SetVisible(PartyFormationWindow, 1)	
	return

def CreateCharPress():
	global PartyFormationWindow, CreateCharWindow
	GemRB.SetVisible(PartyFormationWindow, 0)
	CreateCharWindow = GemRB.LoadWindow(3)
	
	CreateButton = GemRB.GetControl(CreateCharWindow, 0)
	GemRB.SetEvent(CreateCharWindow, CreateButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharCreatePress")
	GemRB.SetControlStatus(CreateCharWindow, CreateButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(CreateCharWindow, CreateButton, 13954)

	DeleteButton = GemRB.GetControl(CreateCharWindow, 3)
	GemRB.SetEvent(CreateCharWindow, DeleteButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharDeletePress")
	GemRB.SetControlStatus(CreateCharWindow, DeleteButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText(CreateCharWindow, DeleteButton, 13957)

	CancelButton = GemRB.GetControl(CreateCharWindow, 4)
	GemRB.SetEvent(CreateCharWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CreateCharCancelPress")
	GemRB.SetControlStatus(CreateCharWindow, CancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(CreateCharWindow, CancelButton, 13727)
	GemRB.SetButtonFlags(CreateCharWindow, CancelButton, IE_GUI_BUTTON_DEFAULT, OP_OR)
	
	GemRB.SetVisible(CreateCharWindow, 1)
	return

def CreateCharCreatePress():
	global PartyFormationWindow, CreateCharWindow
	GemRB.UnloadWindow(CreateCharWindow)
	GemRB.UnloadWindow(PartyFormationWindow)
	GemRB.SetNextScript("CharGen")
	return

def CreateCharDeletePress():
	return

def CreateCharCancelPress():
	global PartyFormationWindow, CreateCharWindow
	GemRB.UnloadWindow(CreateCharWindow)
	GemRB.SetVisible(PartyFormationWindow, 1)
	return

def ModifyCharsPress():
	return

def DonePress():
	return
	
def ExitPress():
	global PartyFormationWindow, ExitWindow
	GemRB.SetVisible(PartyFormationWindow, 0)
	ExitWindow = GemRB.LoadWindow(7)
	
	ExitButton = GemRB.GetControl(ExitWindow, 1)
	GemRB.SetEvent(ExitWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitExitPress")
	GemRB.SetText(ExitWindow, ExitButton, 13906)
	
	CancelButton = GemRB.GetControl(ExitWindow, 2)
	GemRB.SetEvent(ExitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExitCancelPress")
	GemRB.SetText(ExitWindow, CancelButton, 13727)
	
	TextArea = GemRB.GetControl(ExitWindow, 0)
	GemRB.SetText(ExitWindow, TextArea, 11329)

	GemRB.SetVisible(ExitWindow, 1)
	return

def ExitCancelPress():
	global PartyFormationWindow, ExitWindow
	GemRB.UnloadWindow(ExitWindow)
	GemRB.SetVisible(PartyFormationWindow, 1)
	return

def ExitExitPress():
	global PartyFormationWindow, ExitWindow
	GemRB.UnloadWindow(ExitWindow)
	GemRB.UnloadWindow(PartyFormationWindow)
	GemRB.SetNextScript("Start")
	return


import GemRB

StartWindow = 0

def OnLoad():
	global StartWindow
	GemRB.LoadWindowPack("START")
	OptionsWindow = GemRB.LoadWindow(0)
	SinglePlayerButton = GemRB.GetControl(StartWindow, 0)
	ExitButton = GemRB.GetControl(StartWindow, 3)
	OptionsButton = GemRB.GetControl(StartWindow, 4)
	MultiPlayerButton = GemRB.GetControl(StartWindow, 1)
	MoviesButton = GemRB.GetControl(StartWindow, 2)
	DisabledButton = GemRB.GetControl(StartWindow, 5)
	GemRB.CreateLabel(OptionsWindow, 0x0fff0000, 0,450,640,30, "REALMS", "GemRB Ver 0.0.1", 1);
	GemRB.SetControlStatus(StartWindow, DisabledButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MoviesButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetText(StartWindow, OptionsButton, 13905)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetText(StartWindow, DisabledButton, "")
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetEvent(StartWindow, OptionsButton, IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(StartWindow, DisabledButton, IE_GUI_BUTTON_ON_PRESS, "")
	GemRB.SetVisible(StartWindow, 1)
	return
	
def SinglePlayerPress():
	global StartWindow
	
	return
	
def ExitPress():
	global StartWindow
	
	return
	
def OptionsPress():
	global StartWindow
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def MultiPlayerPress():
	global StartWindow
	
	return
	
def MoviesPress():
	global StartWindow
	
	return

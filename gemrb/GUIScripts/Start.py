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
	GemRB.CreateLabel(OptionsWindow, 0xffff0000, 0,450,640,30, "REALMS", "GemRB Ver 0.0.1", 1);
	GemRB.SetControlStatus(StartWindow, DisabledButton, 0x00000003);
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, 0x00000000);
	GemRB.SetControlStatus(StartWindow, ExitButton, 0x00000000);
	GemRB.SetControlStatus(StartWindow, OptionsButton, 0x00000000);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, 0x00000000);
	GemRB.SetControlStatus(StartWindow, MoviesButton, 0x00000000);
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetText(StartWindow, OptionsButton, 13905)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetText(StartWindow, DisabledButton, "")
	GemRB.SetEvent(StartWindow, SinglePlayerButton, 0x00000000, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, ExitButton, 0x00000000, "ExitPress")
	GemRB.SetEvent(StartWindow, OptionsButton, 0x00000000, "OptionsPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, 0x00000000, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, 0x00000000, "MoviesPress")
	GemRB.SetEvent(StartWindow, DisabledButton, 0x00000000, "")
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
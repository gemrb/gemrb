import GemRB

StartWindow = 0
QuitWindow = 0
ExitButton = 0
SinglePlayerButton = 0
OptionsButton = 0
MultiPlayerButton = 0
MoviesButton = 0

def OnLoad():
	global StartWindow, QuitWindow, ExitButton, OptionsButton, MultiPlayerButton, MoviesButton
	GemRB.LoadWindowPack("START")
	QuitWindow = GemRB.LoadWindow(1)
	QuitTextArea = GemRB.GetControl(QuitWindow,0)
	StartWindow = GemRB.LoadWindow(0)
	SinglePlayerButton = GemRB.GetControl(StartWindow, 0)
	ExitButton = GemRB.GetControl(StartWindow, 3)
	OptionsButton = GemRB.GetControl(StartWindow, 4)
	MultiPlayerButton = GemRB.GetControl(StartWindow, 1)
	MoviesButton = GemRB.GetControl(StartWindow, 2)
	DisabledButton = GemRB.GetControl(StartWindow, 5)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,450,640,30, "REALMS", "GemRB Ver 0.0.1", 1);
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
	GemRB.SetText(QuitWindow, QuitTextArea, 19532)
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetEvent(StartWindow, OptionsButton, IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(StartWindow, DisabledButton, IE_GUI_BUTTON_ON_PRESS, "")
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return
	
def SinglePlayerPress():
	
	GemRB.SetText(StartWindow, OptionsButton, "")
	GemRB.SetText(StartWindow, SinglePlayerButton, 13728)
	GemRB.SetText(StartWindow, ExitButton, 15416)
	GemRB.SetText(StartWindow, MultiPlayerButton, 13729)
	GemRB.SetText(StartWindow, MoviesButton, 33093)
	GemRB.SetEvent(StartWindow, MultiPlayerButton, 0, "LoadSingle")
	GemRB.SetEvent(StartWindow, SinglePlayerButton, 0, "NewSingle")
	GemRB.SetEvent(StartWindow, MoviesButton, 0, "Tutorial")
	GemRB.SetEvent(StartWindow, ExitButton, 0, "BackToMain")
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_DISABLED);
	return

def LoadSingle():
	return
def NewSingle():
	return
def Tutorial():
	return
	
def ExitPress():
	GemRB.SetText(StartWindow, SinglePlayerButton, "")
	GemRB.SetText(StartWindow, MultiPlayerButton, "")
	GemRB.SetText(StartWindow, OptionsButton, "")
	GemRB.SetText(StartWindow, MoviesButton, 13727)
	GemRB.SetEvent(StartWindow,SinglePlayerButton,0,"")
	GemRB.SetEvent(StartWindow, ExitButton, 0, "ExitConfirmed")
	GemRB.SetEvent(StartWindow, MoviesButton, 0, "ExitCancelled")
	GemRB.SetEvent(StartWindow,MultiPlayerButton,0,"")
	GemRB.SetEvent(StartWindow,OptionsButton,0,"")
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetVisible(QuitWindow,1)
	return
	
def ExitConfirmed():
	GemRB.SetVisible(QuitWindow,0)
	GemRB.SetVisible(StartWindow,0)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.Quit()
	return

def OptionsPress():
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("StartOpt")
	return
	
def MultiPlayerPress():
	
	return
	
def MoviesPress():
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("GUIMOVIE")
	return

def ExitCancelled():
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, OptionsButton, 13905)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetEvent(StartWindow, OptionsButton, IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return
	
def BackToMain():
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetText(StartWindow, OptionsButton, 13905)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetEvent(StartWindow, OptionsButton, IE_GUI_BUTTON_ON_PRESS, "OptionsPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetVisible(StartWindow, 1)
	return

import GemRB

StartWindow = 0
QuitWindow = 0
ExitButton = 0
SinglePlayerButton = 0
MultiPlayerButton = 0
MoviesButton = 0

def OnLoad():
	global StartWindow, QuitWindow
	global ExitButton, MultiPlayerButton, MoviesButton

	GemRB.LoadWindowPack("START")
#quit subwindow
	QuitWindow = GemRB.LoadWindow(3)
	QuitTextArea = GemRB.GetControl(QuitWindow,0)
	CancelButton = GemRB.GetControl(QuitWindow, 2)
	ConfirmButton = GemRB.GetControl(QuitWindow, 1)
	GemRB.SetText(QuitWindow, QuitTextArea, 19532)
	GemRB.SetText(QuitWindow, CancelButton, 13727)
	GemRB.SetText(QuitWindow, ConfirmButton, 15417)
	GemRB.SetEvent(QuitWindow, ConfirmButton, 0, "ExitConfirmed")
	GemRB.SetEvent(QuitWindow, CancelButton, 0, "ExitCancelled")
#main window
	StartWindow = GemRB.LoadWindow(0)
	SinglePlayerButton = GemRB.GetControl(StartWindow, 0)
	MultiPlayerButton = GemRB.GetControl(StartWindow, 1)
	MoviesButton = GemRB.GetControl(StartWindow, 2)
	ExitButton = GemRB.GetControl(StartWindow, 3)

	#GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,450,640,30, "REALMS", "GemRB Ver 0.0.1", 1);
	
	BackToMain();
	
	GemRB.LoadMusicPL("Theme.mus")
	return
	
def SinglePlayerPress():
	
	GemRB.SetText(StartWindow, SinglePlayerButton, 13728)
	GemRB.SetText(StartWindow, MultiPlayerButton, 13729)
	GemRB.SetText(StartWindow, MoviesButton, 24110)
	GemRB.SetText(StartWindow, ExitButton, 15416)
	GemRB.SetEvent(StartWindow, MultiPlayerButton, 0, "LoadSingle")
	GemRB.SetEvent(StartWindow, SinglePlayerButton, 0, "NewSingle")
	GemRB.SetEvent(StartWindow, MoviesButton, 0, "MissionPack")
	GemRB.SetEvent(StartWindow, ExitButton, 0, "BackToMain")
	return

def MultiPlayerPress():
	
	GemRB.SetText(StartWindow, SinglePlayerButton, 11825)
	GemRB.SetText(StartWindow, MultiPlayerButton, 20642)
	GemRB.SetText(StartWindow, MoviesButton, 15416)
	GemRB.SetText(StartWindow, ExitButton, "")
	GemRB.SetEvent(StartWindow, SinglePlayerButton, 0, "PregenPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, 0, "ConnectPress")
	GemRB.SetEvent(StartWindow, MoviesButton, 0, "BackToMain")
	GemRB.SetEvent(StartWindow, ExitButton, 0, "")
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_DISABLED);
	return

def ConnectPress():
#well...
	return

def PregenPress():
#GemRB.SetPlayMode(-1) #do not start game after chargen
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
        GemRB.SetNextScript("CharGen") #temporarily
	return

def LoadSingle():
	return

def MissionPack():
	return

def NewSingle():
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
#main menu: -1, single player:0, tutorial mode=1, multi-player:2
#GemRB.SetPlayMode(0)
	GemRB.LoadGame(-1)
        GemRB.SetNextScript("CharGen") #temporarily
	return

def PlayPress():
        GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
#main menu: -1, single player:0, tutorial mode=1, multi-player:2
#GemRB.SetPlayMode(1)
	GemRB.SetNextScript("CharGen")
        return

def ExitPress():
	GemRB.SetVisible(StartWindow,0)
	GemRB.SetVisible(QuitWindow,1)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def MoviesPress():
#apparently the order is important
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetNextScript("GUIMOVIE")
	return

def ExitCancelled():
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return
	
def BackToMain():
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MoviesButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return

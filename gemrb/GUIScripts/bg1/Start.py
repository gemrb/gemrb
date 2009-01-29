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

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ('BG4LOGO',1)
		GemRB.PlayMovie ('TSRLOGO',1)
		GemRB.PlayMovie ('BILOGO',1)
		GemRB.PlayMovie ('INFELOGO',1)
		GemRB.PlayMovie ('INTRO',1)
		GemRB.SetVar ("SkipIntroVideos", 1)

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

	BackToMain()
	
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
	if GemRB.GetString(24110) == "": # TODO: better way to detect lack of mission pack?
		GemRB.SetButtonFlags(StartWindow, MoviesButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
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
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(StartWindow, ExitButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	return

def ConnectPress():
#well...
	return

def PregenPress():
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVar("PlayMode",0) #loadgame needs this hack
	GemRB.GameSetExpansion(0)
	GemRB.LoadGame(-1)
	GemRB.SetVar("PlayMode",-1)
	GemRB.SetNextScript("GUIMP")
	return

def LoadSingle():
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVar("PlayMode",0)
	GemRB.GameSetExpansion(0)
	GemRB.SetNextScript("GUILOAD")
	return

def MissionPack():
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVar("PlayMode",3) #use mpsave for saved games
	GemRB.GameSetExpansion(1)
	GemRB.SetNextScript("GUILOAD")
	return

def NewSingle():
	GemRB.UnloadWindow(StartWindow)
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVar("PlayMode",0)
	GemRB.GameSetExpansion(0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(-1)
	GemRB.SetNextScript("CharGen") #temporarily
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
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, MoviesButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(StartWindow, SinglePlayerButton, 15413)
	GemRB.SetText(StartWindow, MultiPlayerButton, 15414)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetEvent(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ON_PRESS, "SinglePlayerPress")
	GemRB.SetEvent(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ON_PRESS, "MultiPlayerPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
	GemRB.SetButtonFlags(StartWindow, MoviesButton, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	GemRB.SetButtonFlags(StartWindow, ExitButton, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return

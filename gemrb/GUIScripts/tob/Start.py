#ToB start window, precedes the SoA window
import GemRB

StartWindow = 0
QuitWindow = 0
ExitButton = 0

def OnLoad():
	global StartWindow
	global ExitButton

	GemRB.LoadWindowPack("START")
#quit subwindow, to be done
#	QuitWindow = GemRB.LoadWindow(7)
#	QuitTextArea = GemRB.GetControl(QuitWindow,0)
#	CancelButton = GemRB.GetControl(QuitWindow, 2)
#	ConfirmButton = GemRB.GetControl(QuitWindow, 1)
#	GemRB.SetText(QuitWindow, QuitTextArea, 19532)
#	GemRB.SetText(QuitWindow, CancelButton, 13727)
#	GemRB.SetText(QuitWindow, ConfirmButton, 15417)
#	GemRB.SetEvent(QuitWindow, ConfirmButton, 0, "ExitConfirmed")
#	GemRB.SetEvent(QuitWindow, CancelButton, 0, "ExitCancelled")
#main window
	StartWindow = GemRB.LoadWindow(7)
	SinglePlayerButton = GemRB.GetControl(StartWindow, 0)
	ExitButton = GemRB.GetControl(StartWindow, 3)
	OptionsButton = GemRB.GetControl(StartWindow, 4)
	MultiPlayerButton = GemRB.GetControl(StartWindow, 1)
	MoviesButton = GemRB.GetControl(StartWindow, 2)
	DisabledButton = GemRB.GetControl(StartWindow, 5)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,450,640,30, "REALMS", "", 1);
	Label=GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, Label,GEMRB_VERSION)
	GemRB.SetControlStatus(StartWindow, DisabledButton, IE_GUI_BUTTON_DISABLED);
	GemRB.SetControlStatus(StartWindow, SinglePlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, ExitButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, OptionsButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MultiPlayerButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetControlStatus(StartWindow, MoviesButton, IE_GUI_BUTTON_ENABLED);
	GemRB.SetText(StartWindow, ExitButton, 15417)
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")
#	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	GemRB.LoadMusicPL("Theme.mus")
	GemRB.StartPL()
	return
	
def ExitPress():
	GemRB.SetVisible(StartWindow,0)
	GemRB.SetVisible(QuitWindow,1)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def ExitCancelled():
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return
	

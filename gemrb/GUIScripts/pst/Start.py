import GemRB

StartWindow = 0
QuitWindow = 0

def OnLoad():
	global StartWindow, QuitWindow
	GemRB.LoadWindowPack("START")
#quit subwindow
	QuitWindow = GemRB.LoadWindow(3)
	QuitTextArea = GemRB.GetControl(QuitWindow,0)
	CancelButton = GemRB.GetControl(QuitWindow, 2)
	ConfirmButton = GemRB.GetControl(QuitWindow, 1)
	GemRB.SetText(QuitWindow, QuitTextArea, 20582)
	GemRB.SetText(QuitWindow, CancelButton, 23789)
	GemRB.SetText(QuitWindow, ConfirmButton, 23787)
	GemRB.SetEvent(QuitWindow, ConfirmButton, IE_GUI_BUTTON_ON_PRESS, "ExitConfirmed")
	GemRB.SetEvent(QuitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExitCancelled")
#main window
	StartWindow = GemRB.LoadWindow(0)
	NewLifeButton = GemRB.GetControl(StartWindow, 0)
	ResumeLifeButton = GemRB.GetControl(StartWindow, 2)
	ExitButton = GemRB.GetControl(StartWindow, 3)
	GemRB.SetEvent(StartWindow, NewLifeButton, IE_GUI_BUTTON_ON_PRESS, "NewLifePress")
	GemRB.SetEvent(StartWindow, ResumeLifeButton, IE_GUI_BUTTON_ON_PRESS, "ResumeLifePress");
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")

	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,415,640,30, "EXOFONT", "", 1);
	Label=GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, Label,GEMRB_VERSION)
	
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	
	GemRB.LoadMusicPL("Main.mus")
	GemRB.StartPL()
	return
	
def NewLifePress():
	GemRB.UnloadWindow(QuitWindow)
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("NewLife")
	return

def ResumeLifePress():
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

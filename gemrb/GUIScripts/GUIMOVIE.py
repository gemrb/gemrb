import GemRB

MovieWindow = 0
TextAreaControl = 0

def OnLoad():
	global MovieWindow, TextAreaControl
#for testing purposes
	GemRB.SetVar("BG2INTRO",1)
	GemRB.SetVar("BG4LOGO",1)

	GemRB.LoadWindowPack("GUIMOVIE")
	MovieWindow = GemRB.LoadWindow(0)
	TextAreaControl = GemRB.GetControl(MovieWindow, 0)
	PlayButton = GemRB.GetControl(MovieWindow, 2)
	CreditsButton = GemRB.GetControl(MovieWindow, 3)
	DoneButton = GemRB.GetControl(MovieWindow, 4)
#GemRB.SetupListBoxFrom2DA(MovieWindow,TextAreaControl, "MOVIDESC")
	GemRB.SetText(MovieWindow, TextAreaControl,"")
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
#see if the movie is allowed to be in the list
		if GemRB.GetVar(t)==1:
			s = GemRB.GetTableValue(MoviesTable, i, 0)
			GemRB.TextAreaAppend(MovieWindow, TextAreaControl, s,-1)
	GemRB.SetText(MovieWindow, PlayButton, 17318)
	GemRB.SetText(MovieWindow, CreditsButton, 15591)
	GemRB.SetText(MovieWindow, DoneButton, 11973)
	GemRB.SetEvent(MovieWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetEvent(MovieWindow, CreditsButton, IE_GUI_BUTTON_ON_PRESS, "CreditsPress")
	GemRB.SetEvent(MovieWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.ShowModal(MovieWindow)
	return
	
def PlayPress():
	return

def CreditsPress():
	return

def DonePress():
	GemRB.UnloadWindow(MovieWindow)
	GemRB.SetNextScript("Start")
	return

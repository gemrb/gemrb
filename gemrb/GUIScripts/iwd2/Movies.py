import GemRB

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE")
	MovieWindow = GemRB.LoadWindow(2)
	TextAreaControl = GemRB.GetControl(MovieWindow, 0)
	GemRB.SetTextAreaSelectable(MovieWindow, TextAreaControl,1)
	PlayButton = GemRB.GetControl(MovieWindow, 2)
	CreditsButton = GemRB.GetControl(MovieWindow, 3)
	DoneButton = GemRB.GetControl(MovieWindow, 4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			s = GemRB.GetTableValue(MoviesTable, i, 0)
			GemRB.TextAreaAppend(MovieWindow, TextAreaControl, s,-1)
	GemRB.SetVarAssoc(MovieWindow, TextAreaControl, "MovieIndex",0)
	GemRB.SetText(MovieWindow, PlayButton, 17318)
	GemRB.SetText(MovieWindow, CreditsButton, 15591)
	GemRB.SetText(MovieWindow, DoneButton, 11973)
	GemRB.SetEvent(MovieWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetEvent(MovieWindow, CreditsButton, IE_GUI_BUTTON_ON_PRESS, "CreditsPress")
	GemRB.SetEvent(MovieWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetVisible(MovieWindow,1)
	return
	
def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = GemRB.GetTableRowName(MoviesTable, i)
				GemRB.PlayMovie(s)
				GemRB.InvalidateWindow(MovieWindow)
				return
			s = s - 1
	return

def CreditsPress():
	return

def DonePress():
	GemRB.UnloadWindow(MovieWindow)
	GemRB.SetNextScript("Options")
	return

import GemRB

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	MovieWindow = GemRB.LoadWindowObject(2)
	MovieWindow.SetFrame ()
	TextAreaControl = MovieWindow.GetControl(0)
	TextAreaControl.SetFlags(IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)
	MoviesTable = GemRB.LoadTableObject("MOVIDESC")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			s = MoviesTable.GetValue(i, 0)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CreditsPress")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DonePress")
	MovieWindow.SetVisible(1)
	return
	
def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = MoviesTable.GetRowName(i)
				GemRB.PlayMovie(s, 1)
				MovieWindow.Invalidate()
				return
			s = s - 1
	return

def CreditsPress():
	if MovieWindow:
		MovieWindow.Unload()
	GemRB.SetNextScript("Songs")
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Unload()
	GemRB.SetNextScript("Options")
	return

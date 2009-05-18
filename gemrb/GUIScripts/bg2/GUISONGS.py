#instead of credits, you can listen the songs of the game :)
import GemRB
from GUICommon import HasTOB

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 640, 480)
	MovieWindow = GemRB.LoadWindowObject(0)
	MovieWindow.SetFrame ()
	TextAreaControl = MovieWindow.GetControl(0)
	TextAreaControl.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)
	MoviesTable = GemRB.LoadTableObject("SONGLIST")
	for i in range(0, MoviesTable.GetRowCount() ):
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
	t = MoviesTable.GetValue(s, 1)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie("endcrdit", 1)
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Unload()
	if HasTOB():
		GemRB.SetToken ("NextScript","Start2")
	else:
		GemRB.SetToken ("NextScript","Start")
	return

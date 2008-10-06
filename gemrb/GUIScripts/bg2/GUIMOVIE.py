import GemRB
from GUICommon import GameIsTOB

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0
PlayButton = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable, PlayButton

	GemRB.LoadWindowPack ("GUIMOVIE", 640, 480)
	MovieWindow = GemRB.LoadWindow (0)
	GemRB.SetWindowFrame (MovieWindow)
	TextAreaControl = GemRB.GetControl (MovieWindow, 0)
	PlayButton = GemRB.GetControl (MovieWindow, 2)
	CreditsButton = GemRB.GetControl (MovieWindow, 3)
	DoneButton = GemRB.GetControl (MovieWindow, 4)
	MoviesTable = GemRB.LoadTable ("MOVIDESC")
	for i in range( GemRB.GetTableRowCount (MoviesTable) ):
		t = GemRB.GetTableRowName (MoviesTable, i)
		if GemRB.GetVar (t)==1:
			s = GemRB.GetTableValue (MoviesTable, i, 0)
			GemRB.TextAreaAppend (MovieWindow, TextAreaControl, s,-1)
	GemRB.SetVarAssoc (MovieWindow, TextAreaControl, "MovieIndex",0)
	GemRB.SetTextAreaFlags (MovieWindow, TextAreaControl, IE_GUI_TEXTAREA_SELECTABLE, OP_NAND)
	GemRB.SetEvent (MovieWindow, TextAreaControl, IE_GUI_TEXTAREA_ON_CHANGE, "MoviePress")
	GemRB.SetText (MovieWindow, PlayButton, 17318)
	GemRB.SetText (MovieWindow, CreditsButton, 15591)
	GemRB.SetText (MovieWindow, DoneButton, 11973)
	GemRB.SetEvent (MovieWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetControlStatus (MovieWindow, PlayButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (MovieWindow, CreditsButton, IE_GUI_BUTTON_ON_PRESS, "CreditsPress")
	GemRB.SetEvent (MovieWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetVisible (MovieWindow,1)
	return

def MoviePress():
	GemRB.SetControlStatus (MovieWindow, PlayButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetTextAreaFlags (MovieWindow, TextAreaControl, IE_GUI_TEXTAREA_SELECTABLE,OP_SET) # show selection
	return

def PlayPress():
	s = GemRB.GetVar ("MovieIndex")
	for i in range( GemRB.GetTableRowCount (MoviesTable) ):
		t = GemRB.GetTableRowName (MoviesTable, i)
		if GemRB.GetVar (t)==1:
			if s==0:
				GemRB.SetControlStatus (MovieWindow, PlayButton, IE_GUI_BUTTON_DISABLED)
				GemRB.SetTextAreaFlags (MovieWindow, TextAreaControl, IE_GUI_TEXTAREA_SELECTABLE,OP_NAND) # hide selection
				s = GemRB.GetTableRowName (MoviesTable, i)
				GemRB.PlayMovie (s, 1)
				GemRB.InvalidateWindow (MovieWindow)
				return
			s = s - 1
	return

def CreditsPress():
	GemRB.UnloadWindow (MovieWindow)
	GemRB.SetNextScript ("GUISONGS")
	return

def DonePress():
	GemRB.UnloadWindow (MovieWindow)
	if GameIsTOB():
		GemRB.SetNextScript ("Start2")
	else:
		GemRB.SetNextScript ("Start")
	return

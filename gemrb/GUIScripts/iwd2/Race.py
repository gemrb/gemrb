#character generation, race (GUICG2)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
SubRacesTable = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, SubRacesTable
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	RaceWindow = GemRB.LoadWindowObject (8)

	RaceTable = GemRB.LoadTableObject ("races")
	RaceCount = RaceTable.GetRowCount ()
	
	SubRacesTable = GemRB.LoadTableObject ("SUBRACES")

	for i in range(2,9):
		Button = RaceWindow.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	for i in range(7):
		Button = RaceWindow.GetControl (i+2)
		Button.SetText (RaceTable.GetValue (i,0) )
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS,"RacePress")
		Button.SetVarAssoc ("BaseRace",RaceTable.GetValue (i, 3) )

	BackButton = RaceWindow.GetControl (11) 
	BackButton.SetText (15416)
	DoneButton = RaceWindow.GetControl (0)
	DoneButton.SetText (36789)
	DoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = RaceWindow.GetControl (9)
	TextAreaControl.SetText (17237)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.SetVisible(1)
	return

def RacePress():
	global RaceWindow, RaceTable, SubRacesTable
	Race = GemRB.GetVar ("BaseRace")
	GemRB.SetVar ("Race", Race)
	RaceID = RaceTable.GetRowName (RaceTable.FindValue (3, Race) )
	HasSubRaces = SubRacesTable.GetValue (RaceID, "PURE_RACE")
	if HasSubRaces == 0:
		TextAreaControl.SetText (RaceTable.GetValue (RaceID,"DESC_REF") )
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
		return
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("SubRaces")
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("CharGen2")
	GemRB.SetVar ("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload ()
	GemRB.SetNextScript ("CharGen3") #class
	return

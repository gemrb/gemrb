#character generation, SubRaces (GUICG54)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
SubRacesTable = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, SubRacesTable
	
	GemRB.LoadWindowPack("GUICG")
	RaceWindow = GemRB.LoadWindow(54)

	RaceTable = GemRB.LoadTable("races")
	RaceCount = GemRB.GetTableRowCount(RaceTable)
	
	SubRacesTable = GemRB.LoadTable("SUBRACES")

	for i in range(1, 5):
		Button = GemRB.GetControl(RaceWindow,i)
		GemRB.SetButtonFlags(RaceWindow,Button,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	Race = GemRB.GetVar("Race")-1
	RaceName = GemRB.GetTableRowName(RaceTable, Race)

	HasSubRace = GemRB.GetTableValue(SubRacesTable, RaceName , "PURE_RACE")
	Button = GemRB.GetControl(RaceWindow,1)
	RaceStrRef = GemRB.GetTableValue(RaceTable, HasSubRace, "CAP_REF")
	GemRB.SetText(RaceWindow,Button, RaceStrRef )
	GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
	RaceID = GemRB.GetTableValue(RaceTable, HasSubRace, "ID")
	GemRB.SetVarAssoc(RaceWindow,Button,"Race",RaceID)
	
	HasSubRace = GemRB.GetTableValue(SubRacesTable, RaceName , "SUBRACE1")
	Button = GemRB.GetControl(RaceWindow,2)
	RaceStrRef = GemRB.GetTableValue(RaceTable, HasSubRace, "CAP_REF")
	GemRB.SetText(RaceWindow,Button, RaceStrRef )
	GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
	RaceID = GemRB.GetTableValue(RaceTable, HasSubRace, "ID")
	GemRB.SetVarAssoc(RaceWindow,Button,"Race",RaceID)
	
	HasSubRace = GemRB.GetTableValue(SubRacesTable, RaceName , "SUBRACE2")
	Button = GemRB.GetControl(RaceWindow,3)
	if HasSubRace != 0:
		RaceStrRef = GemRB.GetTableValue(RaceTable, HasSubRace, "CAP_REF")
		GemRB.SetText(RaceWindow,Button, RaceStrRef )
		GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
		RaceID = GemRB.GetTableValue(RaceTable, HasSubRace, "ID")
		GemRB.SetVarAssoc(RaceWindow,Button,"Race",RaceID)
	else:
		GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(RaceWindow,Button,IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		GemRB.SetText(RaceWindow,Button, "")
	
	HasSubRace = GemRB.GetTableValue(SubRacesTable, RaceName , "SUBRACE3")
	Button = GemRB.GetControl(RaceWindow,4)
	if HasSubRace != 0:
		RaceStrRef = GemRB.GetTableValue(RaceTable, HasSubRace, "CAP_REF")
		GemRB.SetText(RaceWindow,Button, RaceStrRef )
		GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
		RaceID = GemRB.GetTableValue(RaceTable, HasSubRace, "ID")
		GemRB.SetVarAssoc(RaceWindow,Button,"Race",RaceID)
	else:
		GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(RaceWindow,Button,IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		GemRB.SetText(RaceWindow,Button, "")

	BackButton = GemRB.GetControl(RaceWindow,8) 
	GemRB.SetText(RaceWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(RaceWindow,0)
	GemRB.SetText(RaceWindow,DoneButton,11973)
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl = GemRB.GetControl(RaceWindow, 6)
	GemRB.SetText(RaceWindow,TextAreaControl, GemRB.GetTableValue(RaceTable, RaceName, "DESC_REF"))

	GemRB.SetEvent(RaceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(RaceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(RaceWindow,1)
	return

def SubRacePress():
	global RaceWindow, RaceTable, TextAreaControl
	Race = GemRB.GetVar("Race")-1
	print str(Race)
	GemRB.SetText(RaceWindow,TextAreaControl, GemRB.GetTableValue(RaceTable, Race, 1))
	return

def BackPress():
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("CharGen2")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("CharGen3") #class
	return

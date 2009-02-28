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
	
	GemRB.LoadWindowPack("GUICG", 800, 600)
	RaceWindow = GemRB.LoadWindowObject(54)

	RaceTable = GemRB.LoadTableObject("races")
	RaceCount = RaceTable.GetRowCount()
	
	SubRacesTable = GemRB.LoadTableObject("SUBRACES")

	for i in range(1, 5):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		
	Race = GemRB.GetVar("BaseRace")
	RaceName = RaceTable.GetRowName(RaceTable.FindValue(3, Race) )

	PureRace = SubRacesTable.GetValue(RaceName , "PURE_RACE")
	Button = RaceWindow.GetControl(1)
	RaceStrRef = RaceTable.GetValue(PureRace, "CAP_REF")
	Button.SetText(RaceStrRef )
	Button.SetState(IE_GUI_BUTTON_ENABLED)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
	RaceID = RaceTable.GetValue(PureRace, "ID")
	Button.SetVarAssoc("Race",RaceID)
	
	#if you want a fourth subrace you can safely add a control id #5 to
	#the appropriate window (guicg#54), and increase 4 to 5
	for i in range(1,4):
		Label = "SUBRACE"+str(i)
		HasSubRace = SubRacesTable.GetValue(RaceName, Label)
		Button = RaceWindow.GetControl(i+1)
		if HasSubRace == 0:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetText("")
		else:
			HasSubRace = PureRace+"_"+HasSubRace
			RaceStrRef = RaceTable.GetValue(HasSubRace, "CAP_REF")
			Button.SetText(RaceStrRef )
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"SubRacePress")
			RaceID = RaceTable.GetValue(HasSubRace, "ID")
			Button.SetVarAssoc("Race",RaceID)

	BackButton = RaceWindow.GetControl(8) 
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = RaceWindow.GetControl(6)
	TextAreaControl.SetText(RaceTable.GetValue(RaceName, "DESC_REF"))

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.SetVisible(1)
	return

def SubRacePress():
	global RaceWindow, RaceTable, TextAreaControl
	Race = RaceTable.FindValue(3, GemRB.GetVar("Race") )
	TextAreaControl.SetText(RaceTable.GetValue(Race, 1))
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("Race")
	GemRB.SetVar("Race",0)  #scrapping the subrace value
	GemRB.SetVar("BaseRace",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen3") #class
	return

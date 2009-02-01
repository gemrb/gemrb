#character generation, race (GUICG2)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable
	
	GemRB.LoadWindowPack("GUICG")
	RaceWindow = GemRB.LoadWindowObject(8)

	RaceTable = GemRB.LoadTableObject("races")
	RaceCount = RaceTable.GetRowCount()

	for i in range(2,RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	for i in range(2, RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetText(RaceTable.GetValue(i-2,0) )
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RacePress")
		Button.SetVarAssoc("Race",i-1)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17237)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.SetVisible(1)
	return

def RacePress():
	Race = GemRB.GetVar("Race")-1
	TextAreaControl.SetText(RaceTable.GetValue(Race,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen2")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen3") #class
	return

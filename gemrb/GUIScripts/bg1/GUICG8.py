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
	RaceWindow = GemRB.LoadWindow(8)

	RaceTable = GemRB.LoadTable("races")
	RaceCount = GemRB.GetTableRowCount(RaceTable)

	for i in range(2,RaceCount+2):
		Button = GemRB.GetControl(RaceWindow,i)
		GemRB.SetButtonFlags(RaceWindow,Button,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	for i in range(2, RaceCount+2):
		Button = GemRB.GetControl(RaceWindow,i)
		GemRB.SetText(RaceWindow,Button, GemRB.GetTableValue(RaceTable,i-2,0) )
		GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"RacePress")
		GemRB.SetVarAssoc(RaceWindow,Button,"Race",i-1)

	BackButton = GemRB.GetControl(RaceWindow, 10)
	GemRB.SetText(RaceWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(RaceWindow,0)
	GemRB.SetText(RaceWindow,DoneButton,11973)
	GemRB.SetButtonFlags(RaceWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl = GemRB.GetControl(RaceWindow, 8)
	GemRB.SetText(RaceWindow,TextAreaControl,17237)

	GemRB.SetEvent(RaceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(RaceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(RaceWindow,1)
	return

def RacePress():
	Race = GemRB.GetVar("Race")-1
	GemRB.SetText(RaceWindow,TextAreaControl, GemRB.GetTableValue(RaceTable,Race,1) )
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
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

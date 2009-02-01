#character generation, racial enemy (GUICG15)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
RaceCount = 0
TopIndex = 0

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(0, 11):
		Button = RaceWindow.GetControl(i+22)
		Val = RaceTable.GetValue(i+TopIndex,0)
		if Val==0:
			Button.SetText("")
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetText(Val)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RacePress")
			Button.SetVarAssoc("HateRace",RaceTable.GetValue(i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, RaceCount, TopIndex
	
	GemRB.LoadWindowPack("GUICG", 800 ,600)
	RaceWindow = GemRB.LoadWindowObject(15)

	Class = GemRB.GetVar("BaseClass")
	TmpTable = GemRB.LoadTableObject("clskills")
	TableName = TmpTable.GetValue(Class, 0)
	if TableName == "*":
		GemRB.SetNextScript("Skills")
		return
	RaceTable = GemRB.LoadTableObject(TableName)
	RaceCount = RaceTable.GetRowCount()-11
	if RaceCount<0:
		RaceCount=0

	for i in range(0,11):
		Button = RaceWindow.GetControl(i+22)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	BackButton = RaceWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = RaceWindow.GetControl(11)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(8)
	TextAreaControl.SetText(17256)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = RaceWindow.GetControl(1)
	ScrollBarControl.SetVarAssoc("TopIndex",RaceCount)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "DisplayRaces")

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.SetVisible(1)
	DisplayRaces()
	return

def RacePress():
	Race = GemRB.GetVar("HateRace")
 	Row = RaceTable.FindValue(1, Race)
	TextAreaControl.SetText(RaceTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetVar("HateRace",0)  #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("Skills")
	return

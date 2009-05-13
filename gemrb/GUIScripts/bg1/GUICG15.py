#character generation, racial enemy (GUICG15)
import GemRB

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0
RaceCount = 0
TopIndex = 0
#the size of the selection list
LISTSIZE = 6

def DisplayRaces():
	global TopIndex

	TopIndex=GemRB.GetVar("TopIndex")
	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+2)
		Val = RaceTable.GetValue(i+TopIndex,0)
		if Val==0:
			Button.SetText("")
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		else:
			Button.SetText(Val)
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RacePress")
			Button.SetVarAssoc("HatedRace",RaceTable.GetValue(i+TopIndex,1) )
	return

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	global RaceTable, RaceCount, TopIndex

	ClassTable = GemRB.LoadTableObject("classes")
	ClassRow = GemRB.GetVar("Class")-1
	Class = ClassTable.GetValue(ClassRow, 5)
	TmpTable = GemRB.LoadTableObject("clskills")
	ClassName = TmpTable.GetRowName(Class)
	TableName = TmpTable.GetValue(ClassName, "HATERACE")
	if TableName == "*":
		GemRB.SetNextScript("GUICG7")
		return
	GemRB.LoadWindowPack("GUICG")
	RaceWindow = GemRB.LoadWindowObject(15)
	RaceTable = GemRB.LoadTableObject(TableName)
	RaceCount = RaceTable.GetRowCount()-LISTSIZE+1
	if RaceCount<0:
		RaceCount=0

	for i in range(LISTSIZE):
		Button = RaceWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetSprites("GUIHRC",i,0,1,2,3)

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
	Race = GemRB.GetVar("HatedRace")
 	Row = RaceTable.FindValue(1, Race)
	TextAreaControl.SetText(RaceTable.GetValue(Row, 2) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetVar("HatedRace",0) #scrapping the race value
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("GUICG7") #mage spells
	return

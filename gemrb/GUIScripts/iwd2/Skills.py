#character generation, skills (GUICG6)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
CostTable = 0
TopIndex = 0
PointsLeft = 0
Level = 0
ClassColumn = 0

def RedrawSkills():
	global TopIndex

	if PointsLeft == 0:
		GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_ENABLED)

	SumLabel = GemRB.GetControl(SkillWindow, 0x1000000c)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i+1
		SkillName = GemRB.GetTableValue(SkillTable, Pos, 1)
		Label = GemRB.GetControl(SkillWindow, 0x10000001+i)
		GemRB.SetText(SkillWindow, Label, SkillName)

		SkillName=GemRB.GetTableRowName(SkillTable, Pos) #row name
		Untrained=GemRB.GetTableValue(SkillTable, Pos, 3)
		if Untrained==1:
			Ok=1
		else:
			Ok=0

		Button1 = GemRB.GetControl(SkillWindow, i*2+14)
		Button2 = GemRB.GetControl(SkillWindow, i*2+15)
		if Ok == 0:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_DISABLED)
#			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
#			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_ENABLED)
#			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
#			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)

		Label = GemRB.GetControl(SkillWindow, 0x10000069+i)
		ActPoint = GemRB.GetVar("Skill "+str(Pos) )
		GemRB.SetText(SkillWindow, Label, str(ActPoint) )

	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, PointsLeft, KitName, Level, ClassColumn
	
	GemRB.SetVar("Level",1) #for simplicity
	Class = GemRB.GetVar("Class") - 1
	ClassTable = GemRB.LoadTable("classes")
	KitName = GemRB.GetTableRowName(ClassTable, Class)
	ClassColumn = GemRB.GetTableValue(ClassTable, Class, 3) - 1
	if ClassColumn < 0:  #it was already a base class
		ClassColumn = Class
	#classcolumn is base class
	SkillPtsTable = GemRB.LoadTable("skillpts")
	PointsLeft = GemRB.GetTableValue(SkillPtsTable, 0, ClassColumn)

	Level = GemRB.GetVar("Level")
	PointsLeft = PointsLeft * Level
	GemRB.UnloadTable(SkillPtsTable)

	SkillTable = GemRB.LoadTable("skills")
	RowCount = GemRB.GetTableRowCount(SkillTable)

	CostTable = GemRB.LoadTable("skilcost")

	SkillRacTable = GemRB.LoadTable("SKILLRAC")
	RaceTable = GemRB.LoadTable("RACES")
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race")-1)

	for i in range(0,RowCount):
		SkillName = GemRB.GetTableRowName(SkillTable,i)
		if GemRB.GetTableValue(SkillTable,SkillName, KitName)==1:
			b=GemRB.GetTableValue(SkillRacTable, RaceName, SkillName)
			GemRB.SetVar("Skill "+str(i),b)
		else:
			GemRB.SetVar("Skill "+str(i),0)

	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG")
	SkillTable = GemRB.LoadTable("skills")
	SkillWindow = GemRB.LoadWindow(6)

	for i in range(0,10):
		Button = GemRB.GetControl(SkillWindow, i+93)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button = GemRB.GetControl(SkillWindow, i*2+14)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button = GemRB.GetControl(SkillWindow, i*2+15)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

	BackButton = GemRB.GetControl(SkillWindow,105)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,36789)
	GemRB.SetButtonFlags(SkillWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(SkillWindow, 92)
	GemRB.SetText(SkillWindow,TextAreaControl,17248)

	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = GemRB.GetControl(SkillWindow, 91)
	GemRB.SetEvent(SkillWindow, ScrollBarControl,IE_GUI_SCROLLBAR_ON_CHANGE,"ScrollBarPress")
	#decrease it with the number of controls on screen (list size)
	GemRB.SetVarAssoc(SkillWindow, ScrollBarControl, "TopIndex",RowCount-10)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	TopIndex = 0
	RedrawSkills()
	GemRB.SetVisible(SkillWindow,1)
	return


def JustPress():
	Pos = GemRB.GetVar("Skill")+TopIndex+1
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex+1
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex+1
	Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)

	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	if PointsLeft < Cost:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint >= Level:
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - Cost
	RedrawSkills()
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("Feats") #feats
	return

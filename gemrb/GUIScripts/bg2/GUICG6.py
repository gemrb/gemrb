#character generation, (thief) skills (GUICG6)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
TopIndex = 0
PointsLeft = 0
RowCount = 0

def RedrawSkills():
	global TopIndex

	if PointsLeft == 0:
		GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_ENABLED)

	SumLabel = GemRB.GetControl(SkillWindow, 0x10000005)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )

	for i in range(4):
		Pos=TopIndex+i
		SkillName = GemRB.GetTableValue(SkillTable, Pos+2,1)
		Label = GemRB.GetControl(SkillWindow, 0x10000006+i)
		GemRB.SetText(SkillWindow, Label, SkillName)

		SkillName=GemRB.GetTableRowName(SkillTable, Pos+2)
		Ok=GemRB.GetTableValue(SkillTable,SkillName, KitName)
		Button1 = GemRB.GetControl(SkillWindow, i*2+11)
		Button2 = GemRB.GetControl(SkillWindow, i*2+12)
		if Ok == 0:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)

		Label = GemRB.GetControl(SkillWindow, 0x10000001+i)
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
	global SkillTable, PointsLeft, KitName, RowCount
	
	SkillTable = GemRB.LoadTable("skills")
	RowCount = GemRB.GetTableRowCount(SkillTable)-2

	Kit = GemRB.GetVar("Class Kit")
	if Kit != 0:
		KitList = GemRB.LoadTable("kitlist")
		KitName = GemRB.GetTableValue(KitList, Kit, 0) #rowname is just a number
		PointsLeft = GemRB.GetTableValue(SkillTable,"FIRST_LEVEL",KitName)
		if PointsLeft < 0:
			Kit = 0
		
	if Kit == 0:
		ClassTable = GemRB.LoadTable("classes")
		Class = GemRB.GetVar("Class")-1
		KitName = GemRB.GetTableRowName(ClassTable, Class)
	else:
		KitList = GemRB.LoadTable("kitlist")
		KitName = GemRB.GetTableValue(KitList, Kit, 0) #rowname is just a number

	SkillRacTable = GemRB.LoadTable("SKILLRAC")
	RaceTable = GemRB.LoadTable("RACES")
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race")-1)

	Ok=0
	for i in range(RowCount):
		SkillName = GemRB.GetTableRowName(SkillTable,i+2)
		if GemRB.GetTableValue(SkillTable,SkillName, KitName)==1:
			b=GemRB.GetTableValue(SkillRacTable, RaceName, SkillName)
			GemRB.SetVar("Skill "+str(i),b)
			Ok=1
		else:
			GemRB.SetVar("Skill "+str(i),0)

	if Ok==0:  #skipping 
		GemRB.SetNextScript("GUICG9")
		return
	
	Level = GemRB.GetVar("Level")
	PointsLeft = GemRB.GetTableValue(SkillTable,"FIRST_LEVEL",KitName)
	if Level>1:
		PointsLeft = PointsLeft + GemRB.GetTableValue(SkillTable,"RATE",KitName)*(Level-1)

	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG", 640, 480)
	SkillTable = GemRB.LoadTable("skills")
	SkillWindow = GemRB.LoadWindow(6)

	GemRB.SetVar("TopIndex", 0)
	ScrollBarControl = GemRB.GetControl(SkillWindow, 26)
	GemRB.SetEvent(SkillWindow, ScrollBarControl, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	#decrease it with the number of controls on screen (list size)
	GemRB.SetVarAssoc(SkillWindow, ScrollBarControl, "TopIndex", RowCount-3)

	for i in range(4):
		Button = GemRB.GetControl(SkillWindow, i+21)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")
		GemRB.AttachScrollBar (SkillWindow, Button, ScrollBarControl)

		Button = GemRB.GetControl(SkillWindow, i*2+11)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button = GemRB.GetControl(SkillWindow, i*2+12)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill",i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

	BackButton = GemRB.GetControl(SkillWindow,25)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)
	GemRB.SetButtonFlags(SkillWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(SkillWindow, 19)
	GemRB.SetText(SkillWindow,TextAreaControl,17248)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	TopIndex = 0
	RedrawSkills()
	GemRB.SetVisible(SkillWindow,1)
	return


def JustPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos+2,0) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos+2,0) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos+2,0) )
	if PointsLeft == 0:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint >= 200:
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - 1
	RedrawSkills()
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen6")
	for i in range(RowCount):
		GemRB.SetVar("Skill "+str(i), 0)
	return

def NextPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("GUICG9") #weapon proficiencies
	return

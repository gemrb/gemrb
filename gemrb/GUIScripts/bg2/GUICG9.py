#character generation, proficiencies (GUICG9)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
TopIndex = 0
RowCount = 0
PointsLeft = 0

def RedrawSkills():
	SumLabel = GemRB.GetControl(SkillWindow, 0)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )  #points to distribute

	for i in range(0,8):
		Pos=TopIndex+i
		SkillName = GemRB.GetTableValue(SkillTable, Pos+7, 1) #we add the bg1 skill count offset
		GemRB.SetText(SkillWindow, i+1, SkillName)
		ActPoint = GemRB.GetVar("Proficiency "+str(Pos) )
		for j in range(0,5):  #5 is maximum distributable
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			if ActPoint >=j:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
	return

def ScrollBarPress():
	#redraw skill list
	RedrawSkills()
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, PointsLeft
	
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	ClassName = GemRB.GetTableRowName(ClassTable, Class)
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = ClassName
	else:
		KitList = GemRB.LoadTable("kitlist")
		KitName = GemRB.GetTableValue(KitList, Kit, 0) #rowname is just a number

	SkillTable = GemRB.LoadTable("profs")
	Level = GemRB.GetVar("Level")-1
	PointsLeft = GemRB.GetTableValue(SkillTable, ClassName, "FIRST_LEVEL")
	if Level>0:
		PointsLeft=PointsLeft + Level/GemRB.GetTableValue(SkillTable, ClassName, "RATE")

	GemRB.LoadWindowPack("GUICG")
        SkillTable = GemRB.LoadTable("weapprof")
	RowCount = GemRB.GetTableRowCount(SkillTable)-7  #we decrease it with the bg1 skills
	SkillWindow = GemRB.LoadWindow(9)

	for i in range(0,8):
		Button=GemRB.GetControl(SkillWindow, i*2+11)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button=GemRB.GetControl(SkillWindow, i*2+12)
		GemRB.SetVarAssoc(SkillWindow, Button, "Skill", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

		for j in range(0,5):
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			GemRB.SetButtonState(SkillWindow, Star, IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(SkillWindow,77)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(SkillWindow, 19)
	GemRB.SetText(SkillWindow,TextAreaControl,9602)

	ScrollBarControl = GemRB.GetControl(SkillWindow, 78)
	GemRB.SetVarAssoc(SkillWindow, ScrollBarControl, "TopIndex", RowCount)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	TopIndex = 0
	RedrawSkills()
	GemRB.SetVisible(SkillWindow,1)
	return


def RightPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+7, 1) )
	#
	PointsLeft = PointsLeft + 1
	RedrawSkills()
	return

def LeftPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+7, 1) )
	if PointsLeft == 0:
		return
	RedrawSkills()
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen6")
	#scrap skills
	return

def NextPress():
        GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("CharGen7") #appearance
	return

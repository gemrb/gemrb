#character generation, proficiencies (GUICG9)
import GemRB

SkillWindow = 0
TextAreaControl = 0
DoneButton = 0
SkillTable = 0
PointsLeft = 0
ProfColumn = 0
ProfsMax = 0

def RedrawSkills():

	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	SumLabel = SkillWindow.GetControl(0x10000009)
	SumLabel.SetText(str(PointsLeft) )  #points to distribute

	for i in range(8):
		Pos=i
		SkillName = SkillTable.GetValue(Pos, 1)
		MaxProf = SkillTable.GetValue(Pos, ProfColumn)

		Button1=SkillWindow.GetControl(i*2+11)
		Button2=SkillWindow.GetControl(i*2+12)
		if MaxProf == 0:
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			Button1.SetState(IE_GUI_BUTTON_ENABLED)
			Button2.SetState(IE_GUI_BUTTON_ENABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
		
		Label=SkillWindow.GetControl(0x10000001+i)
		Label.SetText(SkillName)

		ActPoint = GemRB.GetVar("Prof "+str(Pos) )
		for j in range(5):  #5 is maximum distributable
			Star=SkillWindow.GetControl(i*5+j+27)
			Star.SetSprites("GUIPFC", 0, 0, 0, 0, 0)
			if ActPoint >j:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton
	global SkillTable, PointsLeft, ProfColumn, ProfsMax
	
	ProfMaxTable = GemRB.LoadTableObject("profsmax")
	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetVar("Class")-1
	ClassID = ClassTable.GetValue(Class, 5)
	#we always use first level column
	ProfsMax = ProfMaxTable.GetValue(ClassID-1, 0)
	KitList = GemRB.LoadTableObject("kitlist")
	Class = ClassTable.FindValue(5, ClassID)
	ClassName = ClassTable.GetRowName(Class)
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = ClassName
		ProfColumn = ClassTable.GetValue(Class,5)+2
	else:
		#rowname is just a number, the kitname is the first data column
		KitName = KitList.GetValue(Kit, 0)
		#this is the proficiency column number in kitlist
		ProfColumn = KitList.GetValue(Kit, 5)

	SkillTable = GemRB.LoadTableObject("profs")
	Level = GemRB.GetVar("Level")-1
	PointsLeft = SkillTable.GetValue(ClassName, "FIRST_LEVEL")
	if Level>0:
		PointsLeft=PointsLeft + Level/SkillTable.GetValue(ClassName, "RATE")

	GemRB.LoadWindowPack("GUICG")
	SkillTable = GemRB.LoadTableObject("weapprof")
	RowCount = SkillTable.GetRowCount()-7  #we decrease it with the bg1 skills
	SkillWindow = GemRB.LoadWindowObject(9)

	for i in range(8):
		Button=SkillWindow.GetControl(i+69)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button=SkillWindow.GetControl(i*2+11)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button=SkillWindow.GetControl(i*2+12)
		Button.SetVarAssoc("Prof", i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "RightPress")

		for j in range(5):
			Star=SkillWindow.GetControl(i*5+j+27)
			Star.SetState(IE_GUI_BUTTON_DISABLED)

	BackButton = SkillWindow.GetControl(77)
	BackButton.SetText(15416)
	DoneButton = SkillWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = SkillWindow.GetControl(68)
	TextAreaControl.SetText(9588)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	RedrawSkills()
	SkillWindow.SetVisible(1)
	return

def JustPress():
	Pos = GemRB.GetVar("Prof")
	TextAreaControl.SetText(SkillTable.GetValue(Pos, 2) )
	return
	
def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")
	TextAreaControl.SetText(SkillTable.GetValue(Pos, 2) )
	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")
	TextAreaControl.SetText(SkillTable.GetValue(Pos, 2) )
	if PointsLeft == 0:
		return
	MaxProf = SkillTable.GetValue(Pos, ProfColumn)
	if MaxProf>ProfsMax:
		MaxProf = ProfsMax

	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint >= MaxProf:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint+1)
	PointsLeft = PointsLeft - 1
	RedrawSkills()
	return

def BackPress():
	if SkillWindow:
		SkillWindow.Unload()
	GemRB.SetNextScript("CharGen6")
	#scrap skills
	return

def NextPress():
	if SkillWindow:
		SkillWindow.Unload()
	GemRB.SetNextScript("CharGen7") #appearance
	return

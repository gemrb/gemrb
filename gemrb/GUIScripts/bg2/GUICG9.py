#character generation, proficiencies (GUICG9)
import GemRB

SkillWindow = 0
TextAreaControl = 0
ScrollBarControl = 0
DoneButton = 0
SkillTable = 0
TopIndex = 0
PointsLeft = 0
ProfColumn = 0

def RedrawSkills(First=0):
	global TopIndex, ScrollBarControl

	if PointsLeft == 0:
		GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_ENABLED)

	SumLabel = GemRB.GetControl(SkillWindow, 0x10000009)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )  #points to distribute

	SkipProfs = []
	for i in range(8):
		Pos=TopIndex+i
		SkillName = GemRB.GetTableValue(SkillTable, Pos+8, 1) #we add the bg1 skill count offset
		MaxProf = GemRB.GetTableValue(SkillTable, Pos+8, ProfColumn) #we add the bg1 skill count offset

		#invalid entry, adjusting scrollbar
		if SkillName == -1:
			GemRB.SetVar("TopIndex",TopIndex)
			GemRB.SetVarAssoc(SkillWindow, ScrollBarControl,"TopIndex",Pos-7)
			break

		Button1=GemRB.GetControl(SkillWindow, i*2+11)
		Button2=GemRB.GetControl(SkillWindow, i*2+12)
		if MaxProf == 0:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			# skip proficiencies only if all the previous ones were skipped too
			if i == 0 or ((i-1) in SkipProfs):
				SkipProfs.append(i)
		else:
			GemRB.SetButtonState(SkillWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(SkillWindow, Button2, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonFlags(SkillWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			GemRB.SetButtonFlags(SkillWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
		
		Label=GemRB.GetControl(SkillWindow, 0x10000001+i)
		GemRB.SetText(SkillWindow, Label, SkillName)

		ActPoint = GemRB.GetVar("Prof "+str(Pos) )
		for j in range(5):  #5 is maximum distributable
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			if ActPoint >j:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				GemRB.SetButtonFlags(SkillWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	# skip unavaliable proficiencies on the first run
	if len(SkipProfs) > 0 and First == 1:
		TopIndex += SkipProfs[len(SkipProfs)-1] + 1
		GemRB.SetVar("TopIndex",TopIndex)
		RedrawSkills()

	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, PointsLeft, ProfColumn, ScrollBarControl
	
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	ClassID = GemRB.GetTableValue(ClassTable, Class, 5)
	KitList = GemRB.LoadTable("kitlist")
	Class = GemRB.FindTableValue(ClassTable, 5, ClassID)
	ClassName = GemRB.GetTableRowName(ClassTable, Class)
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = ClassName
		ProfColumn = GemRB.GetTableValue(ClassTable,Class,5)+2
	else:
		#rowname is just a number, the kitname is the first data column
		KitName = GemRB.GetTableValue(KitList, Kit, 0)
		#this is the proficiency column number in kitlist
		ProfColumn = GemRB.GetTableValue(KitList, Kit, 5)

	SkillTable = GemRB.LoadTable("profs")
	Level = GemRB.GetVar("Level")-1
	PointsLeft = GemRB.GetTableValue(SkillTable, ClassName, "FIRST_LEVEL")
	if Level>0:
		PointsLeft=PointsLeft + Level/GemRB.GetTableValue(SkillTable, ClassName, "RATE")

	GemRB.LoadWindowPack("GUICG", 640, 480)
	SkillTable = GemRB.LoadTable("weapprof")
	RowCount = GemRB.GetTableRowCount(SkillTable)-7  #we decrease it with the bg1 skills
	SkillWindow = GemRB.LoadWindow(9)

	for i in range(8):
		Button=GemRB.GetControl(SkillWindow, i+69)
		GemRB.SetVarAssoc(SkillWindow, Button, "Prof", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button=GemRB.GetControl(SkillWindow, i*2+11)
		GemRB.SetVarAssoc(SkillWindow, Button, "Prof", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button=GemRB.GetControl(SkillWindow, i*2+12)
		GemRB.SetVarAssoc(SkillWindow, Button, "Prof", i)
		GemRB.SetEvent(SkillWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

		for j in range(5):
			Star=GemRB.GetControl(SkillWindow, i*5+j+27)
			GemRB.SetButtonState(SkillWindow, Star, IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(SkillWindow,77)
	GemRB.SetText(SkillWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(SkillWindow,0)
	GemRB.SetText(SkillWindow,DoneButton,11973)
	GemRB.SetButtonFlags(SkillWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(SkillWindow, 68)
	GemRB.SetText(SkillWindow,TextAreaControl,9588)

	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = GemRB.GetControl(SkillWindow, 78)
	GemRB.SetEvent(SkillWindow, ScrollBarControl, IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	ProfCount = RowCount - 8 # decrease it with the number of controls
	# decrease it with the number of invalid proficiencies
	for i in range(RowCount):
		SkillName = GemRB.GetTableValue (SkillTable, i+8, 1)
		if SkillName == -1:
			ProfCount -= 1
	GemRB.SetVarAssoc (SkillWindow, ScrollBarControl, "TopIndex", ProfCount)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	TopIndex = 0
	RedrawSkills(1)
	GemRB.SetVisible(SkillWindow,1)
	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def JustPress():
	Pos = GemRB.GetVar("Prof")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	return
	
def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable, Pos+8, 2) )
	if PointsLeft == 0:
		return
	MaxProf = GemRB.GetTableValue(SkillTable, Pos+8, ProfColumn) #we add the bg1 skill count offset
	if MaxProf>5:
		MaxProf = 5

	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint >= MaxProf:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint+1)
	PointsLeft = PointsLeft - 1
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

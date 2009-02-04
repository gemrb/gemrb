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
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	SumLabel = SkillWindow.GetControl(0x10000009)
	SumLabel.SetText(str(PointsLeft) )  #points to distribute

	SkipProfs = []
	for i in range(8):
		Pos=TopIndex+i
		SkillName = SkillTable.GetValue(Pos+8, 1) #we add the bg1 skill count offset
		MaxProf = SkillTable.GetValue(Pos+8, ProfColumn) #we add the bg1 skill count offset

		#invalid entry, adjusting scrollbar
		if SkillName == -1:
			GemRB.SetVar("TopIndex",TopIndex)
			ScrollBarControl.SetVarAssoc("TopIndex",Pos-7)
			break

		Button1=SkillWindow.GetControl(i*2+11)
		Button2=SkillWindow.GetControl(i*2+12)
		if MaxProf == 0:
			Button1.SetState(IE_GUI_BUTTON_DISABLED)
			Button2.SetState(IE_GUI_BUTTON_DISABLED)
			Button1.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			Button2.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)
			# skip proficiencies only if all the previous ones were skipped too
			if i == 0 or ((i-1) in SkipProfs):
				SkipProfs.append(i)
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
			if ActPoint >j:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
			else:
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	# skip unavaliable proficiencies on the first run
	if len(SkipProfs) > 0 and First == 1:
		TopIndex += SkipProfs[len(SkipProfs)-1] + 1
		GemRB.SetVar("TopIndex",TopIndex)
		RedrawSkills()

	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, PointsLeft, ProfColumn, ScrollBarControl
	
	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetVar("Class")-1
	ClassID = ClassTable.GetValue(Class, 5)
	KitList = GemRB.LoadTableObject("kitlist")
	Class = ClassTable.FindValue(5, ClassID)
	ClassName = ClassTable.GetRowName(Class)
	Kit = GemRB.GetVar("Class Kit")
	SkillTable = GemRB.LoadTableObject ("weapprof")
	if Kit == 0:
		KitName = ClassName
		ProfColumn = SkillTable.GetColumnIndex (ClassName)
	else:
		#rowname is just a number, the kitname is the first data column
		KitName = KitList.GetValue(Kit, 0)
		#this is the proficiency column number in kitlist
		ProfColumn = KitList.GetValue(Kit, 5)

	ProfCountTable = GemRB.LoadTableObject("profs")
	Level = GemRB.GetVar("Level")-1
	PointsLeft = ProfCountTable.GetValue(ClassName, "FIRST_LEVEL")
	if Level>0:
		PointsLeft=PointsLeft + Level/ProfCountTable.GetValue(ClassName, "RATE")

	GemRB.LoadWindowPack("GUICG", 640, 480)
	RowCount = SkillTable.GetRowCount()-7  #we decrease it with the bg1 skills
	SkillWindow = GemRB.LoadWindowObject(9)

	GemRB.SetVar("TopIndex",0)
	ScrollBarControl = SkillWindow.GetControl(78)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, "ScrollBarPress")
	ScrollBarControl.SetDefaultScrollBar ()
	ProfCount = RowCount - 8 # decrease it with the number of controls
	# decrease it with the number of invalid proficiencies
	for i in range(RowCount):
		SkillName = SkillTable.GetValue (i+8, 1)
		if SkillName == -1:
			ProfCount -= 1
	ScrollBarControl.SetVarAssoc ("TopIndex", ProfCount)

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
			#Star.SetState(IE_GUI_BUTTON_DISABLED)

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
	TopIndex = 0
	RedrawSkills(1)
	SkillWindow.SetVisible(1)
	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def JustPress():
	global TextAreaControl
	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+8, 2) )
	return
	
def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+8, 2) )
	ActPoint = GemRB.GetVar("Prof "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Prof "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Prof")+TopIndex
	TextAreaControl.SetText(SkillTable.GetValue(Pos+8, 2) )
	if PointsLeft == 0:
		return
	MaxProf = SkillTable.GetValue(Pos+8, ProfColumn) #we add the bg1 skill count offset
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

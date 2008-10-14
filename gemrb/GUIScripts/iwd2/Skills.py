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
	global CostTable, TopIndex

	SumLabel = GemRB.GetControl(SkillWindow, 0x1000000c)
	if PointsLeft == 0:
		GemRB.SetButtonState(SkillWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetLabelTextColor(SkillWindow,SumLabel, 255, 255, 255)
	else:
		GemRB.SetLabelTextColor(SkillWindow,SumLabel, 255, 255, 0)
	GemRB.SetText(SkillWindow, SumLabel, str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i
		Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)
		# Skill cost is currently restricted to base classes. This means it is fairly easy
		# to add a class kit as it will use its parents skill cost values however it makes
		# it impossible to add a new class entirely. Support for whole new classes is
		# probably not massively important but could eventually allow Prestige classes to
		# be implemented
		SkillName = GemRB.GetTableValue(SkillTable, Pos, 1)
		Label = GemRB.GetControl(SkillWindow, 0x10000001+i)
		ActPoint = GemRB.GetVar("Skill "+str(Pos) )
		if Cost>0:
			#we use this function to retrieve the string
			t=GemRB.StatComment(SkillName,0,0)
			GemRB.SetText(SkillWindow, Label, "%s (%d)"%(t,Cost) )
			GemRB.SetLabelTextColor(SkillWindow, Label, 255, 255, 255)
			if PointsLeft < 1 or ActPoint * Cost >= Level + 3:
				GemRB.SetLabelTextColor(SkillWindow, Label, 150, 150, 150)
		else:
			GemRB.SetText(SkillWindow, Label, SkillName)
			GemRB.SetLabelTextColor(SkillWindow, Label, 150, 150, 150) # Grey

		Button1 = GemRB.GetControl(SkillWindow, i*2+14)
		Button2 = GemRB.GetControl(SkillWindow, i*2+15)
		
		Btn1State = Btn2State = IE_GUI_BUTTON_ENABLED
		if Cost < 1:
			Btn1State = Btn2State = IE_GUI_BUTTON_DISABLED
		else:
			if ActPoint * Cost >= Level + 3 or PointsLeft < 1:
				Btn1State = IE_GUI_BUTTON_DISABLED
			if ActPoint < 1:
				Btn2State = IE_GUI_BUTTON_DISABLED
		GemRB.SetButtonState(SkillWindow, Button1, Btn1State)
		GemRB.SetButtonState(SkillWindow, Button2, Btn2State)

		Label = GemRB.GetControl(SkillWindow, 0x10000069+i)
		GemRB.SetText(SkillWindow, Label, str(ActPoint) )
		if ActPoint>0:
			GemRB.SetLabelTextColor(SkillWindow,Label,0,255,255)
		elif Cost < 1 or PointsLeft < 1:
			GemRB.SetLabelTextColor(SkillWindow,Label, 150, 150, 150)
		else:
			GemRB.SetLabelTextColor(SkillWindow,Label,255,255,255)

	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawSkills()
	return

def OnLoad():
	global SkillWindow, TextAreaControl, DoneButton, TopIndex
	global SkillTable, CostTable, PointsLeft
	global KitName, Level, ClassColumn
	
	#enable repeated clicks
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	GemRB.SetVar("Level",1) #for simplicity
	Class = GemRB.GetVar("Class") - 1
	ClassTable = GemRB.LoadTable("classes")
	KitName = GemRB.GetTableRowName(ClassTable, Class)
	#classcolumn is base class
	ClassColumn=GemRB.GetVar("BaseClass") - 1
	SkillPtsTable = GemRB.LoadTable("skillpts")
	p = GemRB.GetTableValue(SkillPtsTable, 0, ClassColumn)
	IntBonus = GemRB.GetVar("Ability 3")/2-5  #intelligence bonus
	p = p + IntBonus
	#at least 1 skillpoint / level advanced
	if p <1:
		p=1

	Level = GemRB.GetVar("Level")  #this is the level increase
	PointsLeft = p
	
	# Humans recieve +2 skill points at level 1 and +1 skill points each level thereafter
	# Recommend creation of SKILRACE.2da with levels as rows and race names as columns
	
	RaceTable = GemRB.LoadTable("RACES")
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.FindTableValue(RaceTable, 3, GemRB.GetVar('Race')))
	
	### Example code for implementation of SKILRACE.2da
	# TmpTable = GemRB.LoadTable('skilrace')
	# PointsLeft += GemRB.GetTableValue(TmpTable, str(Level), RaceName)
	###
	
	if Level < 2:
		PointsLeft = p * 4 # 4-times skill points @ first level
	
	### Human skill bonus hack (ignores intelligence modifier):
	if RaceName == 'HUMAN':
		if Level < 2: PointsLeft += 2
		else: PointsLeft += 1
	
	GemRB.UnloadTable(SkillPtsTable)

	SkillTable = GemRB.LoadTable("skills")
	RowCount = GemRB.GetTableRowCount(SkillTable)

	CostTable = GemRB.LoadTable("skilcost")

	SkillRacTable = GemRB.LoadTable("SKILLRAC")

	for i in range(0,RowCount):
		GemRB.SetVar("Skill "+str(i),0) # Racial/Class bonuses don't factor in char-gen or leveling
										# so can be safely ignored

	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG", 800 ,600)
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

	ScrollBarControl = GemRB.GetControl(SkillWindow, 104)
	GemRB.SetEvent(SkillWindow, ScrollBarControl,IE_GUI_SCROLLBAR_ON_CHANGE,"ScrollBarPress")
	GemRB.SetDefaultScrollBar (SkillWindow, ScrollBarControl)
	#decrease it with the number of controls on screen (list size)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	GemRB.SetVarAssoc(SkillWindow, ScrollBarControl, "TopIndex",RowCount-10+1)

	GemRB.SetEvent(SkillWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(SkillWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(SkillWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	RedrawSkills()
	GemRB.SetVisible(SkillWindow,1)
	return


def JustPress():
	Pos = GemRB.GetVar("Skill")+TopIndex
	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)
	if Cost==0:
		return

	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Skill "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + Cost
	RedrawSkills()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Skill")+TopIndex
	Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)
	if Cost==0:
		return

	GemRB.SetText(SkillWindow, TextAreaControl, GemRB.GetTableValue(SkillTable,Pos,2) )
	if PointsLeft < Cost:
		return
	ActPoint = GemRB.GetVar("Skill "+str(Pos) )
	if Cost*ActPoint >= Level+3:
		return
	GemRB.SetVar("Skill "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - Cost
	RedrawSkills()
	return

def BackPress():
	GemRB.UnloadWindow(SkillWindow)
	for i in range(GemRB.GetTableRowCount(SkillTable)):
		GemRB.SetVar("Skill "+str(i),0)
	GemRB.SetNextScript("CharGen6")
	return

def NextPress():
	GemRB.UnloadWindow(SkillWindow)
	GemRB.SetNextScript("Feats") #feats
	return

#character generation, ability (GUICG4)
import GemRB

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 16
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	RaceTable = GemRB.LoadTable("races")
	Abracead = GemRB.LoadTable("ABRACEAD")
	RaceID = GemRB.GetVar("Race")
	RowIndex = GemRB.FindTableValue(RaceTable, 3, RaceID)
	RaceName = GemRB.GetTableRowName(RaceTable, RowIndex)

	Minimum = 3
	Maximum = 18

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	tmp = GemRB.GetTableValue(Abclasrq, KitIndex, Abidx)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	Abracerq = GemRB.LoadTable("ABRACERQ")
	Race = GemRB.GetTableRowIndex(Abracerq, RaceName)
	tmp = GemRB.GetTableValue(Abracerq, Race, Abidx*2)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	tmp = GemRB.GetTableValue(Abracerq, Race, Abidx*2+1)
	if tmp!=0 and tmp>Maximum:
		Maximum = tmp

	Race = GemRB.GetTableRowIndex(Abracead, RaceName)
	Add = GemRB.GetTableValue(Abracead, Race, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1

	return

def RollPress():
	global PointsLeft, Add

	GemRB.InvalidateWindow(AbilityWindow)
	GemRB.SetVar("Ability",0)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetLabelTextColor(AbilityWindow,SumLabel, 255, 255, 0)
	PointsLeft=16
	GemRB.SetLabelUseRGB(AbilityWindow, SumLabel, 1)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft))

	for i in range(0,6):
		CalcLimits(i)
		v = 10+Add
		b = v//2-5
		GemRB.SetVar("Ability "+str(i), v )
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		GemRB.SetText(AbilityWindow, Label, str(v) )

		Label = GemRB.GetControl(AbilityWindow, 0x10000024+i)
		GemRB.SetLabelUseRGB(AbilityWindow, Label, 1)
		if b<0:
			GemRB.SetLabelTextColor(AbilityWindow,Label,255,0,0)
		elif b>0:
			GemRB.SetLabelTextColor(AbilityWindow,Label,0,255,0)
		else:
			GemRB.SetLabelTextColor(AbilityWindow,Label,255,255,255)
		GemRB.SetText(AbilityWindow, Label, "%+d"%(b))
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft
	global AbilityTable
	global KitIndex, Minimum, Maximum
	
	#enable repeated clicks
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	Kit = GemRB.GetVar("Class Kit")
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	if Kit == 0:
		KitName = GemRB.GetTableRowName(ClassTable, Class)
	else:
		KitList = GemRB.LoadTable("kitlist")
		#rowname is just a number, first value row what we need here
		KitName = GemRB.GetTableValue(KitList, Kit, 0) 

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	KitIndex = GemRB.GetTableRowIndex(Abclasrq, KitName)

	GemRB.LoadWindowPack("GUICG", 800 ,600)
	AbilityTable = GemRB.LoadTable("ability")
	AbilityWindow = GemRB.LoadWindow(4)

	RollPress()
	for i in range(0,6):
		Button = GemRB.GetControl(AbilityWindow, i+30)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i)

		Button = GemRB.GetControl(AbilityWindow, i*2+16)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i )

		Button = GemRB.GetControl(AbilityWindow, i*2+17)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i )

	BackButton = GemRB.GetControl(AbilityWindow,36)
	GemRB.SetText(AbilityWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AbilityWindow,0)
	GemRB.SetText(AbilityWindow,DoneButton,36789)
	GemRB.SetButtonState(AbilityWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(AbilityWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(AbilityWindow, 29)
	GemRB.SetText(AbilityWindow,TextAreaControl,17247)

	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(AbilityWindow,1)
	return

def RightPress():
	global PointsLeft

	GemRB.InvalidateWindow(AbilityWindow)
	Abidx = GemRB.GetVar("Ability")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	#should be more elaborate
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, Abidx, 1) )
	if Ability<=Minimum:
		return
	Ability -= 1
	GemRB.SetVar("Ability "+str(Abidx), Ability)
	PointsLeft = PointsLeft + 1
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	GemRB.SetLabelTextColor(AbilityWindow,SumLabel, 255, 255, 0)
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	GemRB.SetText(AbilityWindow, Label, str(Ability) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000024+Abidx)
	b = Ability // 2 - 5
	if b<0:
		GemRB.SetLabelTextColor(AbilityWindow,Label,255,0,0)
	elif b>0:
		GemRB.SetLabelTextColor(AbilityWindow,Label,0,255,0)
	else:
		GemRB.SetLabelTextColor(AbilityWindow,Label,255,255,255)
	GemRB.SetText(AbilityWindow, Label, "%+d"%(b))
	GemRB.SetButtonState(AbilityWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	return

def JustPress():
	Abidx = GemRB.GetVar("Ability")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	#should be more elaborate
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, Abidx, 1) )
	return

def LeftPress():
	global PointsLeft

	Abidx = GemRB.GetVar("Ability")
	GemRB.InvalidateWindow(AbilityWindow)
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, Abidx, 1) )
	if PointsLeft == 0:
		return
	if Ability>=Maximum:  #should be more elaborate
		return
	Ability += 1
	GemRB.SetVar("Ability "+str(Abidx), Ability)
	PointsLeft = PointsLeft - 1
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	if PointsLeft == 0:
		GemRB.SetLabelTextColor(AbilityWindow,SumLabel, 255, 255, 255)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	GemRB.SetText(AbilityWindow, Label, str(Ability) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000024+Abidx)
	b = Ability // 2 - 5
	if b<0:
		GemRB.SetLabelTextColor(AbilityWindow,Label,255,0,0)
	elif b>0:
		GemRB.SetLabelTextColor(AbilityWindow,Label,0,255,0)
	else:
		GemRB.SetLabelTextColor(AbilityWindow,Label,255,255,255)
	GemRB.SetText(AbilityWindow, Label, "%+d"%(b))
	if PointsLeft == 0:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	#disable repeated clicks
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen5")
	for i in range(6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen6") #skills
	return

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
HasStrExtra = 0

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	RaceTable = GemRB.LoadTable("races")
	Abracead = GemRB.LoadTable("ABRACEAD")
	Abclsmod = GemRB.LoadTable("ABCLSMOD")
	Race = GemRB.GetVar("Race")-1
	RaceName = GemRB.GetTableRowName(RaceTable, Race)

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
	Add = GemRB.GetTableValue(Abracead, Race, Abidx) + GemRB.GetTableValue(Abclsmod, KitIndex, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1
	if Maximum>25:
		Maximum=25

	return

def RollPress():
	global HasStrExtra, PointsLeft, Add

	GemRB.InvalidateWindow(AbilityWindow)
	GemRB.SetVar("Ability",0)
	GemRB.SetVar("Ability -1",0)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	PointsLeft=16
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft))
	GemRB.SetLabelUseRGB(AbilityWindow, SumLabel, 1)

	if HasStrExtra:
		e = GemRB.Roll(1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)
	for i in range(0,6):
		v = 10+Add
		GemRB.SetVar("Ability "+str(i), v )
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		if i==0 and v==18 and HasStrExtra:
			GemRB.SetText(AbilityWindow, Label, "18/"+str(e) )
		else:
			GemRB.SetText(AbilityWindow, Label, str(v) )
		GemRB.SetLabelUseRGB(AbilityWindow, Label, 1)
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft, HasStrExtra
	global AbilityTable
	global KitIndex, Minimum, Maximum
	
	Kit = GemRB.GetVar("Class Kit")
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	if Kit == 0:
		KitName = GemRB.GetTableRowName(ClassTable, Class)
	else:
		KitList = GemRB.LoadTable("kitlist")
		#rowname is just a number, first value row what we need here
		KitName = GemRB.GetTableValue(KitList, Kit, 0) 

	if GemRB.GetTableValue(ClassTable, Class, 3)=="SAVEWAR":
		HasStrExtra=1
	else:
		HasStrExtra=0

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	KitIndex = GemRB.GetTableRowIndex(Abclasrq, KitName)

	GemRB.LoadWindowPack("GUICG")
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
	GemRB.SetText(AbilityWindow,DoneButton,11973)
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
	GemRB.SetVar("Ability "+str(Abidx), Ability-1)
	PointsLeft = PointsLeft + 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	StrExtra = GemRB.GetVar("StrExtra")
	if Abidx==0 and Ability==19 and StrExtra:
		GemRB.SetText(AbilityWindow, Label, "18/"+str(StrExtra) )
	else:
		GemRB.SetText(AbilityWindow, Label, str(Ability-1) )
	GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_DISABLED)
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
	global PointsLeft, HasStrExtra

	Abidx = GemRB.GetVar("Ability")
	GemRB.InvalidateWindow(AbilityWindow)
	PointsLeft=GemRB.GetVar("Ability -1")
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, Abidx, 1) )
	if PointsLeft == 0:
		return
	if Ability>=Maximum:  #should be more elaborate
		return
	GemRB.SetVar("Ability "+str(Abidx), Ability+1)
	PointsLeft = PointsLeft - 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	StrExtra = GemRB.GetVar("StrExtra")
	if Abidx==0 and Ability==17 and HasStrExtra==1:
		GemRB.SetText(AbilityWindow, Label, "18/"+str(StrExtra) )
	else:
		GemRB.SetText(AbilityWindow, Label, str(Ability+1) )
	if PointsLeft == 0:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen5")
	GemRB.SetVar("StrExtra",0)
	for i in range(-1,6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
        GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen6") #
	return

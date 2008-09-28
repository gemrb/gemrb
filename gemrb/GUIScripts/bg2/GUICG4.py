#character generation, ability (GUICG4)
import GemRB

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 0
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
	global Minimum, Maximum, Add, HasStrExtra, PointsLeft

	GemRB.InvalidateWindow(AbilityWindow)
	GemRB.SetVar("Ability",0)
	GemRB.SetVar("Ability -1",0)
	PointsLeft = 0
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, "0")
	GemRB.SetLabelUseRGB(AbilityWindow, SumLabel, 1)

	if HasStrExtra:
		e = GemRB.Roll(1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)
	for i in range(6):
		dice = 3
		size = 5
		CalcLimits(i)
		v = GemRB.Roll(dice, size, Add+3)
		if v<Minimum:
			v = Minimum
		if v>Maximum:
			v = Maximum
		GemRB.SetVar("Ability "+str(i), v )
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		if i==0 and v==18 and HasStrExtra:
			GemRB.SetText(AbilityWindow, Label, "18/"+str(e) )
		else:
			GemRB.SetText(AbilityWindow, Label, str(v) )
		GemRB.SetLabelUseRGB(AbilityWindow, Label, 1)
	GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
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

	GemRB.LoadWindowPack("GUICG", 640, 480)
	AbilityTable = GemRB.LoadTable("ability")
	AbilityWindow = GemRB.LoadWindow(4)

	RerollButton = GemRB.GetControl(AbilityWindow,2)
	GemRB.SetText(AbilityWindow,RerollButton,11982)
	StoreButton = GemRB.GetControl(AbilityWindow,37)
	GemRB.SetText(AbilityWindow,StoreButton,17373)
	RecallButton = GemRB.GetControl(AbilityWindow,38)
	GemRB.SetText(AbilityWindow,RecallButton,17374)

	BackButton = GemRB.GetControl(AbilityWindow,36)
	GemRB.SetText(AbilityWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AbilityWindow,0)
	GemRB.SetText(AbilityWindow,DoneButton,11973)
	GemRB.SetButtonFlags(AbilityWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	RollPress()
	StorePress()
	for i in range(6):
		Label = GemRB.GetControl(AbilityWindow, i+0x10000009)
		GemRB.SetEvent(AbilityWindow, Label, IE_GUI_LABEL_ON_PRESS, "OverPress"+str(i) )
		Button = GemRB.GetControl(AbilityWindow, i+30)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "EmptyPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i)

		Button = GemRB.GetControl(AbilityWindow, i*2+16)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i )

		Button = GemRB.GetControl(AbilityWindow, i*2+17)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", i )

	TextAreaControl = GemRB.GetControl(AbilityWindow, 29)
	GemRB.SetText(AbilityWindow,TextAreaControl,17247)

	GemRB.SetEvent(AbilityWindow,StoreButton,IE_GUI_BUTTON_ON_PRESS,"StorePress")
	GemRB.SetEvent(AbilityWindow,RecallButton,IE_GUI_BUTTON_ON_PRESS,"RecallPress")
	GemRB.SetEvent(AbilityWindow,RerollButton,IE_GUI_BUTTON_ON_PRESS,"RollPress")
	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(AbilityWindow,1)
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
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
		GemRB.SetText(AbilityWindow, Label, "18/%02d"%(StrExtra) )
	else:
		GemRB.SetText(AbilityWindow, Label, str(Ability+1) )
	if PointsLeft == 0:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def EmptyPress():
	TextAreaControl = GemRB.GetControl(AbilityWindow, 29)
	GemRB.SetText(AbilityWindow,TextAreaControl,17247)
	return

def StorePress():
	GemRB.SetVar("StoredStrExtra",GemRB.GetVar("StrExtra") )
	for i in range(-1,6):
		GemRB.SetVar("Stored "+str(i),GemRB.GetVar("Ability "+str(i) ) )
	return

def RecallPress():
	global PointsLeft

	GemRB.InvalidateWindow(AbilityWindow)
	e=GemRB.GetVar("StoredStrExtra")
	GemRB.SetVar("StrExtra",e)
	for i in range(-1,6):
		v = GemRB.GetVar("Stored "+str(i) )
		GemRB.SetVar("Ability "+str(i), v)
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		if i==0 and v==18 and HasStrExtra==1:
			GemRB.SetText(AbilityWindow, Label, "18/"+str(e) )
		else:
			GemRB.SetText(AbilityWindow, Label, str(v) )

	PointsLeft = GemRB.GetVar("Ability -1")
	if PointsLeft == 0:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_DISABLED)
	return

def BackPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen5")
	GemRB.SetVar("StrExtra",0)
	for i in range(-1,6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	return

def NextPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen6") #
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	return

def OverPress0():
	Ability = GemRB.GetVar("Ability 0")
	CalcLimits(0)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 0, 1) )
	return

def OverPress1():
	Ability = GemRB.GetVar("Ability 1")
	CalcLimits(1)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 1, 1) )
	return

def OverPress2():
	Ability = GemRB.GetVar("Ability 2")
	CalcLimits(2)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 2, 1) )
	return

def OverPress3():
	Ability = GemRB.GetVar("Ability 3")
	CalcLimits(3)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 3, 1) )
	return

def OverPress4():
	Ability = GemRB.GetVar("Ability 4")
	CalcLimits(4)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 4, 1) )
	return

def OverPress5():
	Ability = GemRB.GetVar("Ability 5")
	CalcLimits(5)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable, 5, 1) )
	return

#character generation, ability (GUICG4)
import GemRB

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 0

def RollPress():
	GemRB.InvalidateWindow(AbilityWindow)
	GemRB.SetVar("AbilityIncrease",0)
	GemRB.SetVar("AbilityDecrease",0)
	GemRB.SetVar("Ability -1",0)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, "0")
	GemRB.SetLabelUseRGB(AbilityWindow, SumLabel, 1)

	for i in range(0,6):
		add = 0
		dice = 3
		size = 6
		v = GemRB.Roll(dice, size, add)
		GemRB.SetVar("Ability "+str(i), v )
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		GemRB.SetText(AbilityWindow, Label, str(v) )
		GemRB.SetLabelUseRGB(AbilityWindow, Label, 1)
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft
	global AbilityTable
	
	GemRB.LoadWindowPack("GUICG")
        AbilityTable = GemRB.LoadTable("weapprof")
	AbilityWindow = GemRB.LoadWindow(4)

	RollPress()
	for i in range(0,6):
		Button = GemRB.GetControl(AbilityWindow, i*2+16)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "AbilityIncrease", i )

		Button = GemRB.GetControl(AbilityWindow, i*2+17)
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "AbilityDecrease", i )

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

	TextAreaControl = GemRB.GetControl(AbilityWindow, 29)
	GemRB.SetText(AbilityWindow,TextAreaControl,17247)

	GemRB.SetEvent(AbilityWindow,StoreButton,IE_GUI_BUTTON_ON_PRESS,"StorePress")
	GemRB.SetEvent(AbilityWindow,RecallButton,IE_GUI_BUTTON_ON_PRESS,"RecallPress")
	GemRB.SetEvent(AbilityWindow,RerollButton,IE_GUI_BUTTON_ON_PRESS,"RollPress")
	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(AbilityWindow,1)
	return

def RightPress():
	global PointsLeft

	GemRB.InvalidateWindow(AbilityWindow)
	Abidx = GemRB.GetVar("AbilityDecrease")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	print Ability
	if Ability<4:  #should be more elaborate
		return
	GemRB.SetVar("Ability "+str(Abidx), Ability-1)
	PointsLeft = PointsLeft + 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	GemRB.SetText(AbilityWindow, Label, str(Ability-1) )
	GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_DISABLED)
	return

def LeftPress():
	global PointsLeft

	GemRB.InvalidateWindow(AbilityWindow)
	PointsLeft=GemRB.GetVar("Ability -1")
	if PointsLeft == 0:
		return
	Abidx = GemRB.GetVar("AbilityIncrease")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	if Ability>17:  #should be more elaborate
		return
	GemRB.SetVar("Ability "+str(Abidx), Ability+1)
	PointsLeft = PointsLeft - 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = GemRB.GetControl(AbilityWindow, 0x10000002)
	GemRB.SetText(AbilityWindow, SumLabel, str(PointsLeft) )
	Label = GemRB.GetControl(AbilityWindow, 0x10000003+Abidx)
	GemRB.SetText(AbilityWindow, Label, str(Ability+1) )
	if PointsLeft == 0:
		GemRB.SetButtonState(AbilityWindow, DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def StorePress():
	for i in range(-1,6):
		GemRB.SetVar("Stored "+str(i),GemRB.GetVar("Ability "+str(i) ) )
	return

def RecallPress():
	GemRB.InvalidateWindow(AbilityWindow)
	for i in range(-1,6):
		v = GemRB.GetVar("Stored "+str(i) )
		GemRB.SetVar("Ability "+str(i), v)
		Label = GemRB.GetControl(AbilityWindow, 0x10000003+i)
		GemRB.SetText(AbilityWindow, Label, str(v) )
	return

def BackPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen5")
	for i in range(1,6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
        GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen6") #
	return

#character generation, ability (GUICG4)
import GemRB

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global AbilityTable
	
	GemRB.LoadWindowPack("GUICG")
        AbilityTable = GemRB.LoadTable("weapprof")
	AbilityWindow = GemRB.LoadWindow(4)
	GemRB.SetVar("AbilityIncrease",0)
	GemRB.SetVar("AbilityDecrease",0)

	for i in range(0,6):
		Label = GemRB.GetControl(AbilityWindow, 0x10000002+i)
		v = GemRB.GetVar("Ability "+str(i) )
		GemRB.SetText(AbilityWindow, Label, str(v) )

		Button = GemRB.GetControl(AbilityWindow, i*2+16)
		GemRB.SetText(AbilityWindow, Button, GemRB.GetTableValue(AbilityTable,i,0) )
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "AbilityIncrease", i )

		Button = GemRB.GetControl(AbilityWindow, i*2+16)
		GemRB.SetText(AbilityWindow, Button, GemRB.GetTableValue(AbilityTable,i,0) )
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
	GemRB.SetText(AbilityWindow,TextAreaControl,9602)

	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"StorePress")
	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"RecallPress")
	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(AbilityWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(AbilityWindow,1)
	return

def LeftPress():
	return

def RightPress():
	return

def RollPress():
	return

def StorePress():
	for i in range(0,6):
		GemRB.SetVar("Stored "+str(i),GemRB.GetVar("Ability "+str(i) ) )
	return

def RecallPress():
	for i in range(0,6):
		v = GemRB.GetVar("Stored "+str(i) )
		GemRB.SetVar("Ability "+str(i), v)
		Label = GemRB.GetControl(AbilityWindow, 0x40000002+i)
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

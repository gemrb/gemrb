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

	for i in range(0,6):
		print i
		Button = GemRB.GetControl(AbilityWindow, i*2+20)
		GemRB.SetText(AbilityWindow, Button, GemRB.GetTableValue(AbilityTable,i,0) )
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "AbilityIncrease", i )

		Button = GemRB.GetControl(AbilityWindow, i*2+20)
		GemRB.SetText(AbilityWindow, Button, GemRB.GetTableValue(AbilityTable,i,0) )
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "AbilityDecrease", i )

	BackButton = GemRB.GetControl(AbilityWindow,13)
	GemRB.SetText(AbilityWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AbilityWindow,0)
	GemRB.SetText(AbilityWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(AbilityWindow, 11)
	GemRB.SetText(AbilityWindow,TextAreaControl,9602)

	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(AbilityWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(AbilityWindow,1)
	return

def LeftPress():
	return

def RightPress():
	return

def BackPress():
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Ability",0)  #scrapping the alignment value
	return

def NextPress():
        GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("GUICG12") #appearance
	return

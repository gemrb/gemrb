#character generation, ability (GUICG4)
import GemRB

CharGenWindow = 0
AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global CharGenWindow, AbilityWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG")
        AbilityTable = GemRB.LoadTable("weapprof")
	CharGenWindow = GemRB.LoadWindow(0)
	AbilityWindow = GemRB.LoadWindow(4)

	for i in range(0,7):
        	Button = GemRB.GetControl(CharGenWindow,i)
        	GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	for i in range(0,9):
		print i+2
		Button = GemRB.GetControl(AbilityWindow, i+2)
		GemRB.SetText(AbilityWindow, Button, GemRB.GetTableValue(AbilityTable,i,0) )
		GemRB.SetEvent(AbilityWindow, Button, IE_GUI_BUTTON_ON_PRESS, "AbilityPress")
		GemRB.SetVarAssoc(AbilityWindow, Button, "Ability", GemRB.GetTableValue(AlignmentTable,i,3) )

	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
        GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

        AcceptButton = GemRB.GetControl(CharGenWindow, 8)
        GemRB.SetText(CharGenWindow, AcceptButton, 11962)
        GemRB.SetButtonState(CharGenWindow,AcceptButton,IE_GUI_BUTTON_DISABLED)

        ImportButton = GemRB.GetControl(CharGenWindow, 13)
        GemRB.SetText(CharGenWindow, ImportButton, 13955)
        GemRB.SetButtonState(CharGenWindow,ImportButton,IE_GUI_BUTTON_DISABLED)

        CancelButton = GemRB.GetControl(CharGenWindow, 15)
        GemRB.SetText(CharGenWindow, CancelButton, 8159)
        GemRB.SetButtonState(CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)

        BiographyButton = GemRB.GetControl(CharGenWindow, 16)
        GemRB.SetText(CharGenWindow, BiographyButton, 18003)
        GemRB.SetButtonState(CharGenWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(AbilityWindow,13)
	GemRB.SetText(AbilityWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AbilityWindow,0)
	GemRB.SetText(AbilityWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(AbilityWindow, 11)
	GemRB.SetText(AbilityWindow,TextAreaControl,9602)

	GemRB.SetEvent(AbilityWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AbilityWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetButtonState(AbilityWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(AbilityWindow,1)
	return

def AbilityPress():
	Ability = GemRB.GetVar("Ability")-1
	GemRB.SetText(AbilityWindow, TextAreaControl, GemRB.GetTableValue(AbilityTable,Alignment,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Ability",0)  #scrapping the alignment value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(AbilityWindow)
	GemRB.SetNextScript("GUICG12") #appearance
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(AbilityWindow)
        GemRB.SetNextScript("CharGen")
        return

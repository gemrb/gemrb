#character generation, gender (GUICG1)
import GemRB

CharGenWindow = 0
GenderWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global CharGenWindow, GenderWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG")
	CharGenWindow = GemRB.LoadWindow(0)
	GenderWindow = GemRB.LoadWindow(1)

	for i in range(0,7):
        	Button = GemRB.GetControl(CharGenWindow,i)
        	GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

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

	BackButton = GemRB.GetControl(GenderWindow,6)
	GemRB.SetText(GenderWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(GenderWindow,0)
	GemRB.SetText(GenderWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(GenderWindow, 5)
	GemRB.SetText(GenderWindow,TextAreaControl,17236)

	MaleButton = GemRB.GetControl(GenderWindow,2)
	GemRB.SetButtonFlags(GenderWindow,MaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GemRB.GetControl(GenderWindow,3)
	GemRB.SetButtonFlags(GenderWindow,FemaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	GemRB.SetVarAssoc(GenderWindow,MaleButton,"Gender",1)
	GemRB.SetVarAssoc(GenderWindow,FemaleButton,"Gender",2)
	GemRB.SetEvent(GenderWindow,MaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedMale")
	GemRB.SetEvent(GenderWindow,FemaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedFemale")
	GemRB.SetEvent(GenderWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(GenderWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(GenderWindow,1)
	return

def ClickedMale():
	GemRB.SetText(GenderWindow,TextAreaControl,13083)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	GemRB.SetText(GenderWindow,TextAreaControl,13084)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("GUICG12") #appearance
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(GenderWindow)
        GemRB.SetNextScript("CharGen")
        return

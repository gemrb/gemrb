#character generation, gender (GUICG1)
import GemRB

CharGenWindow = 0
GenderWindow = 0
TextAreaControl = 0
CharGenPhase = 1
DoneButton = 0

def OnLoad():
	global CharGenWindow, GenderWindow, TextAreaControl, CharGenPhase, DoneButton

	CharGenPhase = 1
	GemRB.LoadWindowPack("GUICG")
	GenderWindow = GemRB.LoadWindow(1)
        CharGenWindow = GemRB.LoadWindow(0)
	BackButton = GemRB.GetControl(GenderWindow,0)
	GemRB.SetText(GenderWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(GenderWindow,6)
	GemRB.SetText(GenderWindow,DoneButton,11973)

	MaleButton = GemRB.GetControl(GenderWindow,2)
	GemRB.SetButtonFlags(GenderWindow,MaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GemRB.GetControl(GenderWindow,3)
	GemRB.SetButtonFlags(GenderWindow,FemaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	GemRB.SetVarAssoc(GenderWindow,MaleButton,"Gender",1)
	GemRB.SetVarAssoc(GenderWindow,FemaleButton,"Gender",2)
	GemRB.SetEvent(GenderWindow,MaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedMale")
	GemRB.SetEvent(GenderWindow,FemaleButton,IE_GUI_BUTTON_ON_PRESS,"ClickedFemale")
	GemRB.SetEvent(GenderWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"DonePress")
	GemRB.SetEvent(GenderWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,0)
	GemRB.SetVisible(GenderWindow,1)
	GemRB.ShowModal(GenderWindow)
	return

def DonePress():

	GemRB.SetVisible(GenderWindow,0)
	GemRB.SetVisible(CharGenWindow,1)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	GenderButton = GemRB.GetControl(CharGenWindow,0)
	GemRB.SetText(CharGenWindow,GenderButton,11956)
	GemRB.SetButtonState(CharGenWindow,GenderButton,IE_GUI_BUTTON_DISABLED)

	RaceButton = GemRB.GetControl(CharGenWindow,1)
	GemRB.SetText(CharGenWindow,RaceButton, 11957)
	GemRB.SetButtonState(CharGenWindow,RaceButton,IE_GUI_BUTTON_DISABLED)

	ClassButton = GemRB.GetControl(CharGenWindow,2)
	GemRB.SetText(CharGenWindow,ClassButton, 11959)
	GemRB.SetButtonState(CharGenWindow,ClassButton,IE_GUI_BUTTON_DISABLED)

	AlignmentButton = GemRB.GetControl(CharGenWindow,3)
	GemRB.SetText(CharGenWindow,AlignmentButton, 11958)
	GemRB.SetButtonState(CharGenWindow,AlignmentButton,IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = GemRB.GetControl(CharGenWindow,4)
	GemRB.SetText(CharGenWindow,AbilitiesButton, 11960)
	GemRB.SetButtonState(CharGenWindow,AbilitiesButton,IE_GUI_BUTTON_DISABLED)

	SkillButton = GemRB.GetControl(CharGenWindow,5)
	GemRB.SetText(CharGenWindow,SkillButton, 17372)
	GemRB.SetButtonState(CharGenWindow,SkillButton,IE_GUI_BUTTON_DISABLED)

	AppearanceButton = GemRB.GetControl(CharGenWindow,6)
	GemRB.SetText(CharGenWindow,AppearanceButton, 11961)
	GemRB.SetButtonState(CharGenWindow,AppearanceButton,IE_GUI_BUTTON_DISABLED)

	NameButton = GemRB.GetControl(CharGenWindow,7)
	GemRB.SetText(CharGenWindow,NameButton, 11963)
	GemRB.SetButtonState(CharGenWindow,NameButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(CharGenWindow, 11)
	GemRB.SetText(CharGenWindow, BackButton, 15416)

	AcceptButton = GemRB.GetControl(CharGenWindow, 8)
	GemRB.SetText(CharGenWindow, AcceptButton, 11962)
	GemRB.SetButtonState(CharGenWindow,AcceptButton,IE_GUI_BUTTON_DISABLED)

	ImportButton = GemRB.GetControl(CharGenWindow, 13)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)

        CancelButton = GemRB.GetControl(CharGenWindow, 15)
        GemRB.SetText(CharGenWindow, CancelButton, 13727)

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)

	TextAreaControl= GemRB.GetControl(CharGenWindow,9)
	GemRB.SetText(CharGenWindow, TextAreaControl, 16575)

        GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
        GemRB.SetEvent(CharGenWindow, GenderButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.ShowModal(GenderWindow)
	return
	
def ClickedMale():
	GemRB.SetText(CharGenWindow,TextAreaControl,13083)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def ClickedFemale():
	GemRB.SetText(CharGenWindow,TextAreaControl,13084)
	GemRB.SetButtonState(GenderWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("CharGen")
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(GenderWindow)
	GemRB.SetNextScript("GUICG2") #gender
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(GenderWindow)
        GemRB.SetNextScript("Start")
        return

#character generation, race (GUICG2)
import GemRB

CharGenWindow = 0
RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceCount = 0
RaceTable = 0

def OnLoad():
	global CharGenWindow, RaceWindow, TextAreaControl, DoneButton
	global RaceCount, RaceTable
	
	GemRB.LoadWindowPack("GUICG")
        CharGenWindow = GemRB.LoadWindow(0)
	RaceWindow = GemRB.LoadWindow(2)

        for i in range(0,7):
                Button = GemRB.GetControl(CharGenWindow,i)
                GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	RaceTable = GemRB.LoadTable("races")
	RaceCount = GemRB.GetRowCount(RaceTable)

	for i in range(2,RaceCount+2):
                Button = GemRB.GetControl(RaceWindow,i)
                GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_DISABLED)

	ImportButton = GemRB.GetControl(CharGenWindow,13)
	GemRB.SetButtonState(CharGenWindow, ImportButton, IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(RaceWindow,14)
	GemRB.SetText(RaceWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(RaceWindow,0)
	GemRB.SetText(RaceWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(RaceWindow, 13)
	GemRB.SetText(RaceWindow,TextAreaControl,12135)

	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(RaceWindow,1)
	return

def DonePress():

	GemRB.SetVisible(RaceWindow,0)
	GemRB.SetVisible(CharGenWindow,1)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	RaceButton = GemRB.GetControl(CharGenWindow,0)
	GemRB.SetText(CharGenWindow,RaceButton,11956)
	GemRB.SetButtonState(CharGenWindow,RaceButton,IE_GUI_BUTTON_DISABLED)

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
        GemRB.SetText(CharGenWindow, CancelButton, 8159)

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)

	TextAreaControl = GemRB.GetControl(CharGenWindow,9)
	GemRB.SetText(CharGenWindow, TextAreaControl, 16575)
	
	TextAreaControl = GemRB.GetControl(RaceWindow, 9)
	GemRB.SetText(CharGenWindow, TextAreaControl, 16575)

        GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
        GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
        GemRB.SetEvent(CharGenWindow, RaceButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	return
	
def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("GUICG1")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("GUICG3") #gender
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(RaceWindow)
        GemRB.SetNextScript("CharGen")
        return

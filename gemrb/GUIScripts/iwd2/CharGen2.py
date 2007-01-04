#character generation (GUICG 0)
import GemRB

CharGenWindow = 0
StartOverWindow = 0
TextAreaControl = 0

def OnLoad():
	global CharGenWindow, StartOverWindow, TextAreaControl

	GemRB.SetVar("Race",0) #race

	GemRB.LoadWindowPack("GUICG", 800, 600)
	StartOverWindow  = GemRB.LoadWindow(53)
	YesButton = GemRB.GetControl(StartOverWindow,0)
	GemRB.SetText(StartOverWindow, YesButton, 13912)
	GemRB.SetEvent(StartOverWindow, YesButton, IE_GUI_BUTTON_ON_PRESS,"CancelPress")

	NoButton = GemRB.GetControl(StartOverWindow,1)
	GemRB.SetText(StartOverWindow, NoButton, 13913)
	GemRB.SetEvent(StartOverWindow, NoButton, IE_GUI_BUTTON_ON_PRESS,"NoExitPress")

	TextAreaControl = GemRB.GetControl(StartOverWindow, 2)
	GemRB.SetText(StartOverWindow, TextAreaControl, 40275)

	CharGenWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame( CharGenWindow)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	GemRB.SetButtonPicture(CharGenWindow, PortraitButton, PortraitName,"NOPORTLG")

	GenderButton = GemRB.GetControl(CharGenWindow,0)
	GemRB.SetText(CharGenWindow,GenderButton,11956)
	GemRB.SetButtonState(CharGenWindow,GenderButton,IE_GUI_BUTTON_DISABLED)

	RaceButton = GemRB.GetControl(CharGenWindow,1)
	GemRB.SetText(CharGenWindow,RaceButton, 11957)
	GemRB.SetButtonState(CharGenWindow,RaceButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonFlags(CharGenWindow,RaceButton,IE_GUI_BUTTON_DEFAULT,OP_OR)

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
	GemRB.SetText(CharGenWindow,SkillButton, 11983)
	GemRB.SetButtonState(CharGenWindow,SkillButton,IE_GUI_BUTTON_DISABLED)

	AppearanceButton = GemRB.GetControl(CharGenWindow,6)
	GemRB.SetText(CharGenWindow,AppearanceButton, 11961)
	GemRB.SetButtonState(CharGenWindow,AppearanceButton,IE_GUI_BUTTON_DISABLED)

	NameButton = GemRB.GetControl(CharGenWindow,7)
	GemRB.SetText(CharGenWindow,NameButton, 11963)
	GemRB.SetButtonState(CharGenWindow,NameButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(CharGenWindow, 11)
	GemRB.SetText(CharGenWindow, BackButton, 15416)
	GemRB.SetButtonState(CharGenWindow,BackButton,IE_GUI_BUTTON_ENABLED)

	AcceptButton = GemRB.GetControl(CharGenWindow, 8)
	GemRB.SetText(CharGenWindow, AcceptButton, 28210)
	GemRB.SetButtonState(CharGenWindow,AcceptButton,IE_GUI_BUTTON_DISABLED)

	ImportButton = GemRB.GetControl(CharGenWindow, 13)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState(CharGenWindow,ImportButton,IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(CharGenWindow, 15)
	GemRB.SetText(CharGenWindow, CancelButton, 36788)
	GemRB.SetButtonState(CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)
	GemRB.SetButtonState(CharGenWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl= GemRB.GetControl(CharGenWindow,9)
	GemRB.SetText(CharGenWindow, TextAreaControl, 12135)
	GemRB.TextAreaAppend(CharGenWindow, TextAreaControl,": ")
	if GemRB.GetVar("Gender") == 1:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1050)
	else:
		GemRB.TextAreaAppend(CharGenWindow, TextAreaControl, 1051)

	GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "StartOverPress")
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")
	GemRB.SetEvent(CharGenWindow, RaceButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.SetVisible(CharGenWindow,1)
	return
	
def NextPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript("Race") #race
	return

def StartOverPress():
	GemRB.SetVisible(StartOverWindow,1)
	return

def NoExitPress():
	GemRB.SetVisible(StartOverWindow,0)
	GemRB.SetVisible(CharGenWindow,1)
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript("CharGen")
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(StartOverWindow)
	GemRB.SetNextScript("CharGen") #appearance
	return


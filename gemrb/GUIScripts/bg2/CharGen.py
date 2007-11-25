#character generation (GUICG 0)
import GemRB

CharGenWindow = 0
TextAreaControl = 0
ImportButton = 0
CancelButton = 0

def OnLoad():
	global CharGenWindow, TextAreaControl, ImportButton, CancelButton

	#this is tob specific code!
	if GemRB.GetVar("oldgame")==0:
		GemRB.GameSetExpansion(1)
		GemRB.SetVar ("PlayMode",2)
	else:
		GemRB.GameSetExpansion(0)
		GemRB.SetVar ("PlayMode",0)

	GemRB.SetVar("Gender",0) #gender
	GemRB.SetVar("Race",0) #race
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Class Kit",0) #class
	GemRB.SetVar("Alignment",-1) #alignment

	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame (CharGenWindow)
	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	GenderButton = GemRB.GetControl(CharGenWindow,0)
	GemRB.SetText(CharGenWindow,GenderButton,11956)
	GemRB.SetButtonState(CharGenWindow,GenderButton,IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonFlags(CharGenWindow,GenderButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

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
	GemRB.SetButtonState(CharGenWindow,BackButton,IE_GUI_BUTTON_ENABLED)

	AcceptButton = GemRB.GetControl(CharGenWindow, 8)
	GemRB.SetText(CharGenWindow, AcceptButton, 11962)
	GemRB.SetButtonState(CharGenWindow,AcceptButton,IE_GUI_BUTTON_DISABLED)

	ImportButton = GemRB.GetControl(CharGenWindow, 13)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState(CharGenWindow,ImportButton,IE_GUI_BUTTON_ENABLED)

	CancelButton = GemRB.GetControl(CharGenWindow, 15)
	GemRB.SetText(CharGenWindow, CancelButton, 13727)
	GemRB.SetButtonState(CharGenWindow,CancelButton,IE_GUI_BUTTON_ENABLED)

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)
	GemRB.SetButtonState(CharGenWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl= GemRB.GetControl(CharGenWindow,9)
	GemRB.SetText(CharGenWindow, TextAreaControl, 16575)

	GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GemRB.SetEvent(CharGenWindow, GenderButton, IE_GUI_BUTTON_ON_PRESS, "NextPress")
	GemRB.SetEvent(CharGenWindow, ImportButton, IE_GUI_BUTTON_ON_PRESS, "ImportPress")
	GemRB.SetEvent(CharGenWindow, BiographyButton, IE_GUI_BUTTON_ON_PRESS, "BiographyPress")
	GemRB.SetVisible(CharGenWindow,1)
	return
	
def BiographyPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG23") #biography
	return

def NextPress():
#	GemRB.SetButtonState(CharGenWindow, ImportButton, IE_GUI_BUTTON_DISABLED)
#	GemRB.SetButtonState(CharGenWindow, CancelButton, IE_GUI_BUTTON_DISABLED)
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG1") #gender
	return

def CancelPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("Start")
	return

def ImportPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("GUICG24") #import
	return


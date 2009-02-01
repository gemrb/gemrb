#character generation (GUICG 0)
import GemRB

CharGenWindow = 0
TextAreaControl = 0
ImportButton = 0
CancelButton = 0

def OnLoad():
	global CharGenWindow, TextAreaControl, ImportButton, CancelButton

	GemRB.SetVar("Gender",0) #gender
	GemRB.SetVar("Race",0) #race
	GemRB.SetVar("Class",0) #class
	GemRB.SetVar("Alignment",0) #alignment

	GemRB.LoadWindowPack("GUICG")
	CharGenWindow = GemRB.LoadWindowObject(0)
	PortraitButton = CharGenWindow.GetControl(12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	InfoAreaControl = CharGenWindow.GetControl(9)
	InfoAreaControl.SetText(16575)
	
	GenderButton = CharGenWindow.GetControl(0)
	GenderButton.SetText(11956)
	GenderButton.SetState(IE_GUI_BUTTON_ENABLED)
	GenderButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	RaceButton = CharGenWindow.GetControl(1)
	RaceButton.SetText(11957)
	RaceButton.SetState(IE_GUI_BUTTON_DISABLED)

	ClassButton = CharGenWindow.GetControl(2)
	ClassButton.SetText(11959)
	ClassButton.SetState(IE_GUI_BUTTON_DISABLED)

	AlignmentButton = CharGenWindow.GetControl(3)
	AlignmentButton.SetText(11958)
	AlignmentButton.SetState(IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = CharGenWindow.GetControl(4)
	AbilitiesButton.SetText(11960)
	AbilitiesButton.SetState(IE_GUI_BUTTON_DISABLED)

	SkillButton = CharGenWindow.GetControl(5)
	SkillButton.SetText(17372)
	SkillButton.SetState(IE_GUI_BUTTON_DISABLED)

	AppearanceButton = CharGenWindow.GetControl(6)
	AppearanceButton.SetText(11961)
	AppearanceButton.SetState(IE_GUI_BUTTON_DISABLED)

	NameButton = CharGenWindow.GetControl(7)
	NameButton.SetText(11963)
	NameButton.SetState(IE_GUI_BUTTON_DISABLED)

	BackButton = CharGenWindow.GetControl(11)
	BackButton.SetState(IE_GUI_BUTTON_ENABLED)

	AcceptButton = CharGenWindow.GetControl(8)
	AcceptButton.SetText(11962)
	AcceptButton.SetState(IE_GUI_BUTTON_DISABLED)

	ImportButton = CharGenWindow.GetControl(13)
	ImportButton.SetText(13955)
	ImportButton.SetState(IE_GUI_BUTTON_ENABLED)

	CancelButton = CharGenWindow.GetControl(15)
	CancelButton.SetText(13727)
	CancelButton.SetState(IE_GUI_BUTTON_ENABLED)

	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	# this button does nothing when you click it at this stage:
	# BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	GenderButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "NextPress")
	ImportButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ImportPress")
	CharGenWindow.SetVisible(1)
	return
	
def NextPress():
	ImportButton.SetState(IE_GUI_BUTTON_DISABLED)
	CancelButton.SetState(IE_GUI_BUTTON_DISABLED)
	GemRB.DrawWindows()   #needed to redraw the windows NOW
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("GUICG1") #gender
	return

def CancelPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("Start")
	return

def ImportPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetToken("NextScript","CharGen")
	GemRB.SetNextScript("ImportFile") #import
	return


#Character Generation
import GemRB


CharGenWindow = 0
CharGenState = 0
TextArea = 0
PortraitButton = 0

GenderButton = 0
GenderWindow = 0
GenderTextArea = 0
GenderDoneButton = 0

Portrait = 0
PortraitTable = 0
PortraitPortraitButton = 0

RaceButton = 0
RaceWindow = 0
RaceTable = 0
RaceTextArea = 0
RaceDoneButton = 0

ClassButton = 0
ClassWindow = 0
ClassTable = 0
ClassTextArea = 0
ClassDoneButton = 0

AlignmentButton = 0
AlignmentWindow = 0
AlignmentTable = 0
AlignmentTextArea = 0
AlignmentDoneButton = 0

AbilitiesButton = 0
AbilitiesWindow = 0
AbilitiesTable = 0
AbilitiesRaceAddTable = 0
AbilitiesRaceReqTable = 0
AbilitiesClassReqTable = 0
AbilitiesMinimum = 0
AbilitiesMaximum = 0
AbilitiesModifier = 0
AbilitiesTextArea = 0
AbilitiesRecallButton = 0
AbilitiesDoneButton = 0

SkillsButton = 0
ProficienciesWindow = 0
ProficienciesTable = 0
ProficienciesTextArea = 0
ProficienciesDoneButton = 0

SkillsWindow = 0
SkillsTextArea = 0
SkillsDoneButton = 0

AppearanceButton = 0
AppearanceWindow = 0

BiographyButton = 0
BiographyWindow = 0

NameButton = 0
NameWindow = 0


def OnLoad():
	global CharGenWindow, CharGenState, TextArea, PortraitButton
	global GenderButton, RaceButton, ClassButton, AlignmentButton, AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton

	GemRB.LoadWindowPack("GUICG")
	CharGenWindow = GemRB.LoadWindow(0)
	CharGenState = 0

        GenderButton = GemRB.GetControl(CharGenWindow, 0)
	GemRB.SetButtonState(CharGenWindow, GenderButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(CharGenWindow, GenderButton, IE_GUI_BUTTON_ON_PRESS, "GenderPress")
	GemRB.SetText(CharGenWindow, GenderButton, 11956)

        RaceButton = GemRB.GetControl(CharGenWindow, 1)
	GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, RaceButton, IE_GUI_BUTTON_ON_PRESS, "RacePress")
	GemRB.SetText(CharGenWindow, RaceButton, 11957)

        ClassButton = GemRB.GetControl(CharGenWindow, 2)
	GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, ClassButton, IE_GUI_BUTTON_ON_PRESS, "ClassPress")
	GemRB.SetText(CharGenWindow, ClassButton, 11959)

        AlignmentButton = GemRB.GetControl(CharGenWindow, 3)
	GemRB.SetButtonState(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
	GemRB.SetText(CharGenWindow, AlignmentButton, 11958)

        AbilitiesButton = GemRB.GetControl(CharGenWindow, 4)
	GemRB.SetButtonState(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesPress")
	GemRB.SetText(CharGenWindow, AbilitiesButton, 11960)

        SkillsButton = GemRB.GetControl(CharGenWindow, 5)
	GemRB.SetButtonState(CharGenWindow, SkillsButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, SkillsButton, IE_GUI_BUTTON_ON_PRESS, "SkillsPress")
	GemRB.SetText(CharGenWindow, SkillsButton, 11983)

        AppearanceButton = GemRB.GetControl(CharGenWindow, 6)
	GemRB.SetButtonState(CharGenWindow, AppearanceButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, AppearanceButton, IE_GUI_BUTTON_ON_PRESS, "AppearancePress")
	GemRB.SetText(CharGenWindow, AppearanceButton, 11961)

        BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetButtonState(CharGenWindow, BiographyButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, BiographyButton, IE_GUI_BUTTON_ON_PRESS, "BiographyPress")
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)

        NameButton = GemRB.GetControl(CharGenWindow, 7)
	GemRB.SetButtonState(CharGenWindow, NameButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, NameButton, IE_GUI_BUTTON_ON_PRESS, "NamePress")
	GemRB.SetText(CharGenWindow, NameButton, 11963)

	BackButton = GemRB.GetControl(CharGenWindow, 11)
	GemRB.SetButtonState(CharGenWindow, BackButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")

	PortraitButton = GemRB.GetControl(CharGenWindow, 12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	ImportButton = GemRB.GetControl(CharGenWindow, 13)
	GemRB.SetButtonState(CharGenWindow, ImportButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)
	GemRB.SetEvent(CharGenWindow, ImportButton, IE_GUI_BUTTON_ON_PRESS, "ImportPress")

	CancelButton = GemRB.GetControl(CharGenWindow, 15)
	GemRB.SetButtonState(CharGenWindow, CancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(CharGenWindow, CancelButton, 13727)
	GemRB.SetEvent(CharGenWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "CancelPress")

	BackButton = GemRB.GetControl(CharGenWindow, 11)
	GemRB.SetButtonState(CharGenWindow, BackButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "BackPress")

	AcceptButton = GemRB.GetControl(CharGenWindow, 8)
	GemRB.SetButtonState(CharGenWindow, AcceptButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetText(CharGenWindow, AcceptButton, 11962)
	GemRB.SetEvent(CharGenWindow, AcceptButton, IE_GUI_BUTTON_ON_PRESS, "AcceptPress")

	TextArea = GemRB.GetControl(CharGenWindow, 9)
	GemRB.SetText(CharGenWindow, TextArea, 16575)

	GemRB.SetVisible(CharGenWindow, 1)
	return

def BackPress():
	global CharGenWindow, CharGenState
	global GenderButton, RaceButton, ClassButton, AlignmentButton, AbilitiesButton, SkillsButton, AppearanceButton, BiographyButton, NameButton
	if CharGenState > 0:
		CharGenState = CharGenState - 1
	if CharGenState == 0:
		GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, GenderButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 1:
		GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 2:
		GemRB.SetButtonState(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 3:
		GemRB.SetButtonState(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 4:
		GemRB.SetButtonState(CharGenWindow, SkillsButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 5:
		GemRB.SetButtonState(CharGenWindow, AppearanceButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, SkillsButton, IE_GUI_BUTTON_ENABLED)
	elif CharGenState == 6:
		GemRB.SetButtonState(CharGenWindow, NameButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, BiographyButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonState(CharGenWindow, AppearanceButton, IE_GUI_BUTTON_ENABLED)
	return


# Gender Selection

def GenderPress():
	global CharGenWindow, GenderWindow, GenderDoneButton, GenderTextArea
	GemRB.SetVisible(CharGenWindow, 0)
	GenderWindow = GemRB.LoadWindow(1)
	GemRB.SetVar("Gender", 0)

	MaleButton = GemRB.GetControl(GenderWindow, 2)
	GemRB.SetButtonState(GenderWindow, MaleButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(GenderWindow, MaleButton, IE_GUI_BUTTON_ON_PRESS, "MalePress")

	FemaleButton = GemRB.GetControl(GenderWindow, 3)
	GemRB.SetButtonState(GenderWindow, FemaleButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(GenderWindow, FemaleButton, IE_GUI_BUTTON_ON_PRESS, "FemalePress")

	GenderTextArea = GemRB.GetControl(GenderWindow, 5)
	GemRB.SetText(GenderWindow, GenderTextArea, 17236)

	GenderDoneButton = GemRB.GetControl(GenderWindow, 0)
	GemRB.SetButtonState(GenderWindow, GenderDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(GenderWindow, GenderDoneButton, IE_GUI_BUTTON_ON_PRESS, "GenderDonePress")
	GemRB.SetText(GenderWindow, GenderDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, GenderDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GenderCancelButton = GemRB.GetControl(GenderWindow, 6)
	GemRB.SetButtonState(GenderWindow, GenderCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(GenderWindow, GenderCancelButton, IE_GUI_BUTTON_ON_PRESS, "GenderCancelPress")
	GemRB.SetText(GenderWindow, GenderCancelButton, 13727)
	
	GemRB.SetVisible(GenderWindow, 1)
	return

def MalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea
	GemRB.SetVar("Gender", 1)
	GemRB.SetText(GenderWindow, GenderTextArea, 13083)
	GemRB.SetButtonState(GenderWindow, GenderDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def FemalePress():
	global GenderWindow, GenderDoneButton, GenderTextArea
	GemRB.SetVar("Gender", 2)
	GemRB.SetText(GenderWindow, GenderTextArea, 13084)
	GemRB.SetButtonState(GenderWindow, GenderDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def GenderDonePress():
	global CharGenWindow, TextArea, GenderWindow, RaceButton
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetText(CharGenWindow, TextArea, 12135)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, ": ")
	if GemRB.GetVar("Gender") == 1:
		GemRB.TextAreaAppend(CharGenWindow, TextArea, 1050)
	else:
		GemRB.TextAreaAppend(CharGenWindow, TextArea, 1051)
	GemRB.SetVisible(CharGenWindow, 1)
	PortraitSelect()
	return
	
def GenderCancelPress():
	global CharGenWindow, GenderWindow, Gender
	GemRB.SetVar("Gender", 0)
	GemRB.UnloadWindow(GenderWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return

def PortraitSelect():
	global CharGenWindow, PortraitWindow, Portrait, PortraitPortraitButton, PortraitTable
	GemRB.SetVisible(CharGenWindow, 0)
	PortraitWindow = GemRB.LoadWindow(11)

	# this is not the correct one, but I don't know which is
	PortraitTable = GemRB.LoadTable("PICTURES")
	Portrait = 0;
	
	PortraitPortraitButton = GemRB.GetControl(PortraitWindow, 1)
	GemRB.SetButtonFlags(PortraitWindow, PortraitPortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE, OP_SET)

	PortraitLeftButton = GemRB.GetControl(PortraitWindow, 2)
	GemRB.SetButtonState(PortraitWindow, PortraitLeftButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PortraitWindow, PortraitLeftButton, IE_GUI_BUTTON_ON_PRESS, "PortraitLeftPress")
	
	PortraitRightButton = GemRB.GetControl(PortraitWindow, 3)
	GemRB.SetButtonState(PortraitWindow, PortraitRightButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PortraitWindow, PortraitRightButton, IE_GUI_BUTTON_ON_PRESS, "PortraitRightPress")

	PortraitCustomButton = GemRB.GetControl(PortraitWindow, 6)
	GemRB.SetButtonState(PortraitWindow, PortraitCustomButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PortraitWindow, PortraitCustomButton, IE_GUI_BUTTON_ON_PRESS, "PortraitCustomPress")
	GemRB.SetText(PortraitWindow, PortraitCustomButton, 17545)

	PortraitDoneButton = GemRB.GetControl(PortraitWindow, 0)
	GemRB.SetButtonState(PortraitWindow, PortraitDoneButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PortraitWindow, PortraitDoneButton, IE_GUI_BUTTON_ON_PRESS, "PortraitDonePress")
	GemRB.SetText(PortraitWindow, PortraitDoneButton, 11973)
	GemRB.SetButtonFlags(PortraitWindow, PortraitDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	PortraitCancelButton = GemRB.GetControl(PortraitWindow, 5)
	GemRB.SetButtonState(PortraitWindow, PortraitCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(PortraitWindow, PortraitCancelButton, IE_GUI_BUTTON_ON_PRESS, "PortraitCancelPress")
	GemRB.SetText(PortraitWindow, PortraitCancelButton, 13727)

	while GemRB.GetTableValue(PortraitTable, Portrait, 0) != GemRB.GetVar("Gender"):
		Portrait = Portrait + 1
	GemRB.SetButtonPicture(PortraitWindow, PortraitPortraitButton, GemRB.GetTableRowName(PortraitTable, Portrait) + "G")

	GemRB.SetVisible(PortraitWindow, 1)
	return

def PortraitLeftPress():
	global PortraitWindow, Portrait, PortraitTable, PortraitPortraitButton
	while True:
		Portrait = Portrait - 1
		if Portrait < 0:
			Portrait = GemRB.GetTableRowCount(PortraitTable) - 1
		if GemRB.GetTableValue(PortraitTable, Portrait, 0) == GemRB.GetVar("Gender"):
			GemRB.SetButtonPicture(PortraitWindow, PortraitPortraitButton, GemRB.GetTableRowName(PortraitTable, Portrait) + "G")
			return
	
def PortraitRightPress():
	global PortraitWindow, Portrait, PortraitTable, PortraitPortraitButton
	while True:
		Portrait = Portrait + 1
		if Portrait == GemRB.GetTableRowCount(PortraitTable):
			Portrait = 0
		if GemRB.GetTableValue(PortraitTable, Portrait, 0) == GemRB.GetVar("Gender"):
			GemRB.SetButtonPicture(PortraitWindow, PortraitPortraitButton, GemRB.GetTableRowName(PortraitTable, Portrait) + "G")
			return

def PortraitCustomPress():
	return

def PortraitDonePress():
	global CharGenWindow, CharGenState, PortraitWindow, PortraitButton, GenderButton, RaceButton
	GemRB.UnloadWindow(PortraitWindow)
	GemRB.SetButtonPicture(CharGenWindow, PortraitButton, GemRB.GetTableRowName(PortraitTable, Portrait) + "L")
	GemRB.SetButtonState(CharGenWindow, GenderButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_ENABLED)
	CharGenState = 1
	GemRB.SetVisible(CharGenWindow, 1)
	return

def PortraitCancelPress():
	global CharGenWindow, PortraitWindow
	GemRB.UnloadWindow(PortraitWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Race Selection

def RacePress():
	global CharGenWindow, RaceWindow, RaceDoneButton, RaceTable, RaceTextArea
	GemRB.SetVisible(CharGenWindow, 0)
	RaceWindow = GemRB.LoadWindow(8)
	RaceTable = GemRB.LoadTable("RACES")
	GemRB.SetVar("Race", 0)

	for i in range(2, 8):
		RaceSelectButton = GemRB.GetControl(RaceWindow, i)
		GemRB.SetButtonFlags(RaceWindow, RaceSelectButton, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	
	for i in range(2, 8):
		RaceSelectButton = GemRB.GetControl(RaceWindow, i)
		GemRB.SetButtonState(RaceWindow, RaceSelectButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(RaceWindow, RaceSelectButton, IE_GUI_BUTTON_ON_PRESS, "RaceSelectPress")
		GemRB.SetText(RaceWindow, RaceSelectButton, GemRB.GetTableValue(RaceTable, i - 2, 0))
		GemRB.SetVarAssoc(RaceWindow, RaceSelectButton, "Race", i - 1)

	RaceTextArea = GemRB.GetControl(RaceWindow, 8)
	GemRB.SetText(RaceWindow, RaceTextArea, 17237)

	RaceDoneButton = GemRB.GetControl(RaceWindow, 0)
	GemRB.SetButtonState(RaceWindow, RaceDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(RaceWindow, RaceDoneButton, IE_GUI_BUTTON_ON_PRESS, "RaceDonePress")
	GemRB.SetText(RaceWindow, RaceDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, RaceDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	RaceCancelButton = GemRB.GetControl(RaceWindow, 10)
	GemRB.SetButtonState(RaceWindow, RaceCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(RaceWindow, RaceCancelButton, IE_GUI_BUTTON_ON_PRESS, "RaceCancelPress")
	GemRB.SetText(RaceWindow, RaceCancelButton, 13727)

	GemRB.SetVisible(RaceWindow, 1)
	return

def RaceSelectPress():
        global RaceWindow, RaceDoneButton, RaceTable, RaceTextArea
	Race = GemRB.GetVar("Race") - 1
	GemRB.SetText(RaceWindow, RaceTextArea, GemRB.GetTableValue(RaceTable, Race, 1) )
        GemRB.SetButtonState(RaceWindow, RaceDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def RaceDonePress():
	global CharGenWindow, CharGenState, TextArea, RaceWindow, RaceTable, RaceButton, ClassButton
	GemRB.UnloadWindow(RaceWindow)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, 1048, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, ": ")
	GemRB.TextAreaAppend(CharGenWindow, TextArea, GemRB.GetTableValue(RaceTable, GemRB.GetVar("Race") - 1, 2) )
	GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_ENABLED)
	CharGenState = 2
	GemRB.SetVisible(CharGenWindow, 1)
	return

def BackToRacePress():
	GemRB.SetVar("Race", 0)
	GemRB.SetButtonState(CharGenWindow, RaceButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(CharGenWindow, BackButton, IE_GUI_BUTTON_ON_PRESS, "OnLoad")
	return

def RaceCancelPress():
	global CharGenWindow, RaceWindow
	GemRB.SetVar("Race", 0)
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return

# Class Selection

def ClassPress():
	global CharGenWindow, ClassWindow, ClassTable, ClassTextArea, ClassDoneButton
	GemRB.SetVisible(CharGenWindow, 0)
	ClassWindow = GemRB.LoadWindow(2)
	ClassTable = GemRB.LoadTable("CLASSES")
	ClassCount = GemRB.GetTableRowCount(ClassTable)
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race") - 1)
	GemRB.SetVar("Class", 0)

	for i in range(2, 10):
		ClassSelectButton = GemRB.GetControl(ClassWindow, i)
		GemRB.SetButtonFlags(ClassWindow, ClassSelectButton, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)

	HasMulti = 0
	for i in range(0, ClassCount):
		Allowed = GemRB.GetTableValue(ClassTable, GemRB.GetTableRowName(ClassTable, i), RaceName)
		if GemRB.GetTableValue(ClassTable, i, 4):
			if Allowed != 0:
				HasMulti = 1
		else:
			ClassSelectButton = GemRB.GetControl(ClassWindow, i + 2)
			if Allowed > 0:
				GemRB.SetButtonState(ClassWindow, ClassSelectButton, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState(ClassWindow, ClassSelectButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetEvent(ClassWindow, ClassSelectButton, IE_GUI_BUTTON_ON_PRESS,  "ClassSelectPress")
			GemRB.SetText(ClassWindow, ClassSelectButton, GemRB.GetTableValue(ClassTable, i, 0) )
			GemRB.SetVarAssoc(ClassWindow, ClassSelectButton , "Class", i + 1)

        ClassMultiButton = GemRB.GetControl(ClassWindow, 10)
	if HasMulti == 0:
		GemRB.SetButtonState(ClassWindow, ClassMultiButton, IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetButtonState(ClassWindow, ClassMultiButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(ClassWindow, ClassMultiButton, IE_GUI_BUTTON_ON_PRESS, "ClassMultiPress")
	GemRB.SetText(ClassWindow, ClassMultiButton, 11993)
	
        ClassSpecialButton = GemRB.GetControl(ClassWindow, 11)
	GemRB.SetButtonState(ClassWindow, ClassSpecialButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(ClassWindow, ClassSpecialButton, IE_GUI_BUTTON_ON_PRESS, "ClassSpecialPress")
	GemRB.SetText(ClassWindow, ClassSpecialButton, 11994)

	ClassDoneButton = GemRB.GetControl(ClassWindow, 0)
	GemRB.SetButtonState(ClassWindow, ClassDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(ClassWindow, ClassDoneButton, IE_GUI_BUTTON_ON_PRESS, "ClassDonePress")
	GemRB.SetText(ClassWindow, ClassDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, ClassDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	ClassCancelButton = GemRB.GetControl(ClassWindow, 14)
	GemRB.SetButtonState(ClassWindow, ClassCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(ClassWindow, ClassCancelButton, IE_GUI_BUTTON_ON_PRESS, "ClassCancelPress")
	GemRB.SetText(ClassWindow, ClassCancelButton, 13727)

	ClassTextArea = GemRB.GetControl(ClassWindow, 13)
	GemRB.SetText(ClassWindow, ClassTextArea, 17242)

	GemRB.SetVisible(ClassWindow, 1)
	return

def ClassSelectPress():
	global ClassWindow, ClassTable, ClassTextArea, ClassDoneButton
	Class = GemRB.GetVar("Class") - 1
	GemRB.SetText(ClassWindow, ClassTextArea, GemRB.GetTableValue(ClassTable, Class, 1) )
	GemRB.SetButtonState(ClassWindow, ClassDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def ClassMultiPress():
	return

def ClassSpecialPress():
	return

def ClassDonePress():
	global CharGenWindow, CharGenState, ClassWindow, ClassButton, AlignmentButton
	GemRB.UnloadWindow(ClassWindow)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, 12136, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, ": ")
	GemRB.TextAreaAppend(CharGenWindow, TextArea, GemRB.GetTableValue(ClassTable, GemRB.GetVar("Class") - 1, 2) )
	GemRB.SetButtonState(CharGenWindow, ClassButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonState(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_ENABLED)
	CharGenState = 3
	GemRB.SetVisible(CharGenWindow, 1)
	return

def ClassCancelPress():
	global CharGenWindow, ClassWindow
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Alignment Selection

def AlignmentPress():
	global CharGenWindow, AlignmentWindow, AlignmentTable, AlignmentTextArea, AlignmentDoneButton
	GemRB.SetVisible(CharGenWindow, 0)
	AlignmentWindow = GemRB.LoadWindow(3)
	AlignmentTable = GemRB.LoadTable("ALIGNS")
	ClassAlignmentTable = GemRB.LoadTable("ALIGNMNT")
	ClassName = GemRB.GetTableRowName(ClassTable, GemRB.GetVar("Class") - 1)
	GemRB.SetVar("Alignment", 0)
	
	for i in range (2, 11):
		AlignmentSelectButton = GemRB.GetControl(AlignmentWindow, i)
		GemRB.SetButtonFlags(AlignmentWindow, AlignmentSelectButton, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (0, 9):
		AlignmentSelectButton = GemRB.GetControl(AlignmentWindow, i + 2)
		if GemRB.GetTableValue(ClassAlignmentTable, ClassName, GemRB.GetTableValue(AlignmentTable, i, 4)) == 0:
			GemRB.SetButtonState(AlignmentWindow, AlignmentSelectButton, IE_GUI_BUTTON_DISABLED)
		else:
			GemRB.SetButtonState(AlignmentWindow, AlignmentSelectButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(AlignmentWindow, AlignmentSelectButton, IE_GUI_BUTTON_ON_PRESS, "AlignmentSelectPress")
		GemRB.SetText(AlignmentWindow, AlignmentSelectButton, GemRB.GetTableValue(AlignmentTable, i, 0) )
		GemRB.SetVarAssoc(AlignmentWindow, AlignmentSelectButton, "Alignment", i + 1)

	AlignmentTextArea = GemRB.GetControl(AlignmentWindow, 11)
	GemRB.SetText(AlignmentWindow, AlignmentTextArea, 9602)

	AlignmentDoneButton = GemRB.GetControl(AlignmentWindow, 0)
	GemRB.SetButtonState(AlignmentWindow, AlignmentDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(AlignmentWindow, AlignmentDoneButton, IE_GUI_BUTTON_ON_PRESS, "AlignmentDonePress")
	GemRB.SetText(AlignmentWindow, AlignmentDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, AlignmentDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	AlignmentCancelButton = GemRB.GetControl(AlignmentWindow, 13)
	GemRB.SetButtonState(AlignmentWindow, AlignmentCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(AlignmentWindow, AlignmentCancelButton, IE_GUI_BUTTON_ON_PRESS, "AlignmentCancelPress")
	GemRB.SetText(AlignmentWindow, AlignmentCancelButton, 13727)

	GemRB.SetVisible(AlignmentWindow, 1)
	return


def AlignmentSelectPress():
	global AlignmentWindow, AlignmentTable, AlignmentTextArea, AlignmentDoneButton
	Alignment = GemRB.GetVar("Alignment") - 1 
	GemRB.SetText(AlignmentWindow, AlignmentTextArea, GemRB.GetTableValue(AlignmentTable, Alignment, 1))
	GemRB.SetButtonState(AlignmentWindow, AlignmentDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def AlignmentDonePress():
	global CharGenWindow, CharGenState, TextArea, AlignmentWindow, AlignmentTable, AlignmentButton, AbilitiesButton
	GemRB.UnloadWindow(AlignmentWindow)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, 1049, -1)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, ": ")
	GemRB.TextAreaAppend(CharGenWindow, TextArea, GemRB.GetTableValue(AlignmentTable, GemRB.GetVar("Alignment") - 1, 2) )
	GemRB.SetButtonState(CharGenWindow, AlignmentButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonState(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_ENABLED)
	CharGenState = 4
	GemRB.SetVisible(CharGenWindow, 1)
	return

def AlignmentCancelPress():
	global CharGenWindow, AlignmentWindow
	GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Abilities Selection

def AbilitiesPress():
	global CharGenWindow, AbilitiesWindow, AbilitiesTable, AbilitiesRaceAddTable, AbilitiesRaceReqTable, AbilitiesClassReqTable, AbilitiesTextArea, AbilitiesRecallButton, AbilitiesDoneButton
	GemRB.SetVisible(CharGenWindow, 0)
	AbilitiesWindow = GemRB.LoadWindow(4)
	AbilitiesTable = GemRB.LoadTable("ABILITY")
	AbilitiesRaceAddTable = GemRB.LoadTable("ABRACEAD")
	AbilitiesRaceReqTable = GemRB.LoadTable("ABRACERQ")
	AbilitiesClassReqTable = GemRB.LoadTable("ABCLASRQ")

	PointsLeftLabel = GemRB.GetControl(AbilitiesWindow, 0x10000002)
	GemRB.SetLabelUseRGB(AbilitiesWindow, PointsLeftLabel, 1)
	
	for i in range (0, 6):
		AbilitiesLabelButton = GemRB.GetControl(AbilitiesWindow, 30 + i)
		GemRB.SetButtonState(AbilitiesWindow, AbilitiesLabelButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(AbilitiesWindow, AbilitiesLabelButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesLabelPress")
		GemRB.SetVarAssoc(AbilitiesWindow, AbilitiesLabelButton, "AbilityIndex", i + 1)

		AbilitiesPlusButton = GemRB.GetControl(AbilitiesWindow, 16 + i * 2)
		GemRB.SetButtonState(AbilitiesWindow, AbilitiesPlusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(AbilitiesWindow, AbilitiesPlusButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesPlusPress")
		GemRB.SetVarAssoc(AbilitiesWindow, AbilitiesPlusButton, "AbilityIndex", i + 1)

		AbilitiesMinusButton = GemRB.GetControl(AbilitiesWindow, 17 + i * 2)
		GemRB.SetButtonState(AbilitiesWindow, AbilitiesMinusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(AbilitiesWindow, AbilitiesMinusButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesMinusPress")
		GemRB.SetVarAssoc(AbilitiesWindow, AbilitiesMinusButton, "AbilityIndex", i + 1)

		AbilityLabel = GemRB.GetControl(AbilitiesWindow, 0x10000003 + i)
		GemRB.SetLabelUseRGB(AbilitiesWindow, AbilityLabel, 1)

	AbilitiesRerollPress()

	AbilitiesStoreButton = GemRB.GetControl(AbilitiesWindow, 37)
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesStoreButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(AbilitiesWindow, AbilitiesStoreButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesStorePress")
	GemRB.SetText(AbilitiesWindow, AbilitiesStoreButton, 17373)
	
	AbilitiesRecallButton = GemRB.GetControl(AbilitiesWindow, 38)
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesRecallButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(AbilitiesWindow, AbilitiesRecallButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesRecallPress")
	GemRB.SetText(AbilitiesWindow, AbilitiesRecallButton, 17374)
	
	AbilitiesRerollButton = GemRB.GetControl(AbilitiesWindow,2)
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesRerollButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(AbilitiesWindow, AbilitiesRerollButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesRerollPress")
	GemRB.SetText(AbilitiesWindow, AbilitiesRerollButton, 11982)

	AbilitiesTextArea = GemRB.GetControl(AbilitiesWindow, 29)
	GemRB.SetText(AbilitiesWindow, AbilitiesTextArea, 17247)

	AbilitiesDoneButton = GemRB.GetControl(AbilitiesWindow, 0)
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesDoneButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(AbilitiesWindow, AbilitiesDoneButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesDonePress")
	GemRB.SetText(AbilitiesWindow, AbilitiesDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, AbilitiesDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	AbilitiesCancelButton = GemRB.GetControl(AbilitiesWindow, 36)
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(AbilitiesWindow, AbilitiesCancelButton, IE_GUI_BUTTON_ON_PRESS, "AbilitiesCancelPress")
	GemRB.SetText(AbilitiesWindow, AbilitiesCancelButton, 13727)

	GemRB.SetVisible(AbilitiesWindow, 1)
	return

def AbilitiesCalcLimits(Index):
	global RaceTable, AbilitiesRaceReqTable, AbilitiesRaceAddTable, ClassTable, AbilitiesClassReqTable
	global AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race") - 1)
	Race = GemRB.GetTableRowIndex(AbilitiesRaceReqTable, RaceName)
	AbilitiesMinimum = GemRB.GetTableValue(AbilitiesRaceReqTable, Race, Index * 2)
	AbilitiesMaximum = GemRB.GetTableValue(AbilitiesRaceReqTable, Race, Index * 2 + 1)
	AbilitiesModifier = GemRB.GetTableValue(AbilitiesRaceAddTable, Race, Index)

	ClassName = GemRB.GetTableRowName(ClassTable, GemRB.GetVar("Class") - 1)
	Class = GemRB.GetTableRowIndex(AbilitiesClassReqTable, ClassName)
	Min = GemRB.GetTableValue(AbilitiesClassReqTable, Class, Index)
	if Min > 0 and AbilitiesMinimum < Min:
		AbilitiesMinimum = Min

	AbilitiesMinimum = AbilitiesMinimum + AbilitiesModifier
	AbilitiesMaximum = AbilitiesMaximum + AbilitiesModifier
	return

def AbilitiesLabelPress():
	global AbilitiesWindow, AbilitiesTable, AbilitiesTextArea
	AbilityIndex = GemRB.GetVar("AbilityIndex") - 1
	AbilitiesCalcLimits(AbilityIndex)
	GemRB.SetToken("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken("MAXIMUM", str(AbilitiesMaximum) )
	GemRB.SetText(AbilitiesWindow, AbilitiesTextArea, GemRB.GetTableValue(AbilitiesTable, AbilityIndex, 1) )
	return

def AbilitiesPlusPress():
	global AbilitiesWindow, AbilitiesTable, AbilitiesTextArea, AbilitiesMinimum, AbilitiesMaximum
	AbilityIndex = GemRB.GetVar("AbilityIndex") - 1
	AbilitiesCalcLimits(AbilityIndex)
	GemRB.SetToken("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken("MAXIMUM", str(AbilitiesMaximum) )
	GemRB.SetText(AbilitiesWindow, AbilitiesTextArea, GemRB.GetTableValue(AbilitiesTable, AbilityIndex, 1) )
	PointsLeft = GemRB.GetVar("Ability0")
	Ability = GemRB.GetVar("Ability" + str(AbilityIndex + 1) )
	if PointsLeft > 0 and Ability < AbilitiesMaximum:
		PointsLeft = PointsLeft - 1
		GemRB.SetVar("Ability0", PointsLeft)
		PointsLeftLabel = GemRB.GetControl(AbilitiesWindow, 0x10000002)
		GemRB.SetText(AbilitiesWindow, PointsLeftLabel, str(PointsLeft) )
		Ability = Ability + 1
		GemRB.SetVar("Ability" + str(AbilityIndex + 1), Ability)
		AbilityLabel = GemRB.GetControl(AbilitiesWindow, 0x10000003 + AbilityIndex)
		GemRB.SetText(AbilitiesWindow, AbilityLabel, str(Ability) )
		if PointsLeft == 0:
			GemRB.SetButtonState(AlignmentWindow, AbilitiesDoneButton, IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesMinusPress():
	global AbilitiesWindow, AbilitiesTable, AbilitiesTextArea, AbilitiesMinimum, AbilitiesMaximum
	AbilityIndex = GemRB.GetVar("AbilityIndex") - 1
	AbilitiesCalcLimits(AbilityIndex)
	GemRB.SetToken("MINIMUM", str(AbilitiesMinimum) )
	GemRB.SetToken("MAXIMUM", str(AbilitiesMaximum) )
	GemRB.SetText(AbilitiesWindow, AbilitiesTextArea, GemRB.GetTableValue(AbilitiesTable, AbilityIndex, 1) )
	PointsLeft = GemRB.GetVar("Ability0")
	Ability = GemRB.GetVar("Ability" + str(AbilityIndex + 1) )
	if Ability > AbilitiesMinimum:
		Ability = Ability - 1
		GemRB.SetVar("Ability" + str(AbilityIndex + 1), Ability)
		AbilityLabel = GemRB.GetControl(AbilitiesWindow, 0x10000003 + AbilityIndex)
		GemRB.SetText(AbilitiesWindow, AbilityLabel, str(Ability) )
		PointsLeft = PointsLeft + 1
		GemRB.SetVar("Ability0", PointsLeft)
		PointsLeftLabel = GemRB.GetControl(AbilitiesWindow, 0x10000002)
		GemRB.SetText(AbilitiesWindow, PointsLeftLabel, str(PointsLeft) )
		GemRB.SetButtonState(AlignmentWindow, AbilitiesDoneButton, IE_GUI_BUTTON_DISABLED)
	return

def AbilitiesStorePress():
	global AbilitiesWindow, AbilitiesRecallButton
	for i in range(0, 7):
		GemRB.SetVar("Stored" + str(i), GemRB.GetVar("Ability" + str(i)))
	GemRB.SetButtonState(AbilitiesWindow, AbilitiesRecallButton, IE_GUI_BUTTON_ENABLED)
	return

def AbilitiesRecallPress():
	global AbilitiesWindow
	GemRB.InvalidateWindow(AbilitiesWindow)
	for i in range(0, 7):
		GemRB.SetVar("Ability" + str(i), GemRB.GetVar("Stored" + str(i)) )
		AbilityLabel = GemRB.GetControl(AbilitiesWindow, 0x10000002 + i)
		GemRB.SetText(AbilitiesWindow, AbilityLabel, str(GemRB.GetVar("Ability" + str(i))) )
	return

def AbilitiesRerollPress():
	global AbilitiesWindow, AbilitiesMinimum, AbilitiesMaximum, AbilitiesModifier
	GemRB.InvalidateWindow(AbilitiesWindow)
	GemRB.SetVar("Ability0", 0)
	PointsLeftLabel = GemRB.GetControl(AbilitiesWindow, 0x10000002)
	GemRB.SetText(AbilitiesWindow, PointsLeftLabel, "0")
	Dices = 3
	Sides = 6
	for i in range(0, 6):
		AbilitiesCalcLimits(i)
		Value = GemRB.Roll(Dices, Sides, AbilitiesModifier)
		if Value < AbilitiesMinimum:
			Value = AbilitiesMinimum
		if Value > AbilitiesMaximum:
			Value = AbilitiesMaximum
		GemRB.SetVar("Ability" + str(i + 1), Value)
		AbilityLabel = GemRB.GetControl(AbilitiesWindow, 0x10000003 + i)
		GemRB.SetText(AbilitiesWindow, AbilityLabel, str(Value) )
	return

def AbilitiesDonePress():
	global CharGenWindow, CharGenState, TextArea, AbilitiesWindow, AbilitiesTable, AbilitiesButton, SkillsButton
	GemRB.UnloadWindow(AbilitiesWindow)
	GemRB.TextAreaAppend(CharGenWindow, TextArea, "", -1)
	for i in range(0, 6):
		GemRB.TextAreaAppend(CharGenWindow, TextArea, GemRB.GetTableValue(AbilitiesTable, i, 2), -1)
		GemRB.TextAreaAppend(CharGenWindow, TextArea, ": " )
		GemRB.TextAreaAppend(CharGenWindow, TextArea, str(GemRB.GetVar("Ability" + str(i + 1))) )
	GemRB.SetButtonState(CharGenWindow, AbilitiesButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonState(CharGenWindow, SkillsButton, IE_GUI_BUTTON_ENABLED)
	CharGenState = 5
	GemRB.SetVisible(CharGenWindow, 1)
	return

def AbilitiesCancelPress():
	global CharGenWindow, AbilitiesWindow
	GemRB.UnloadWindow(AbilitiesWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Skills Selection

def SkillsPress():
	global CharGenWindow, ProficienciesWindow, ProficienciesTable, ProficienciesTextArea, ProficienciesDoneButton, RaceTable, ClassTable
	GemRB.SetVisible(CharGenWindow, 0)
	ProficienciesWindow = GemRB.LoadWindow(9)
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race") - 1)
	ClassName = GemRB.GetTableRowName(ClassTable, GemRB.GetVar("Class") - 1)
	ProficienciesTable = GemRB.LoadTable("PROFCNCS")
	ClassWeaponsTable = GemRB.LoadTable("CLASWEAP")

	PointsLeftLabel = GemRB.GetControl(ProficienciesWindow, 0x10000009)
	GemRB.SetLabelUseRGB(ProficienciesWindow, PointsLeftLabel, 1)

	for i in range (0,8):
		ProficienciesLabel = GemRB.GetControl(ProficienciesWindow, 69 + i)
		GemRB.SetButtonState(ProficienciesWindow, ProficienciesLabel, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesLabel, IE_GUI_BUTTON_ON_PRESS, "ProficienciesLabelPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesLabel, "ProficienciesIndex", i + 1)

		for j in range (0, 5):
			ProficienciesMark = GemRB.GetControl(ProficienciesWindow, 27 + i * 5 + j)
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMark, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesMark, IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = GemRB.GetTableValue(ClassWeaponsTable, ClassName, GemRB.GetTableRowName(ProficienciesTable, i))

		ProficienciesPlusButton = GemRB.GetControl(ProficienciesWindow, 11 + i * 2)
		if Allowed == 0:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesPlusPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesPlusButton, "ProficienciesIndex", i + 1)

		ProficienciesMinusButton = GemRB.GetControl(ProficienciesWindow, 12 + i * 2)
		if Allowed == 0:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesMinusPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesMinusButton, "ProficienciesIndex", i + 1)

	for i in range (0,7):
		ProficienciesLabel = GemRB.GetControl(ProficienciesWindow, 85 + i)
		GemRB.SetButtonState(ProficienciesWindow, ProficienciesLabel, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesLabel, IE_GUI_BUTTON_ON_PRESS, "ProficienciesLabelPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesLabel, "ProficienciesIndex", i + 9)

		for j in range (0, 5):
			ProficienciesMark = GemRB.GetControl(ProficienciesWindow, 92 + i * 5 + j)
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMark, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesMark, IE_GUI_BUTTON_NO_IMAGE, OP_OR)

		Allowed = GemRB.GetTableValue(ClassWeaponsTable, ClassName, GemRB.GetTableRowName(ProficienciesTable, i + 8))
		
		ProficienciesPlusButton = GemRB.GetControl(ProficienciesWindow, 127 + i * 2)
		if Allowed == 0:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesPlusButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesPlusPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesPlusButton, "ProficienciesIndex", i + 9)

		ProficienciesMinusButton = GemRB.GetControl(ProficienciesWindow, 128 + i * 2)
		if Allowed == 0:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			GemRB.SetButtonState(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ProficienciesWindow, ProficienciesMinusButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesMinusPress")
		GemRB.SetVarAssoc(ProficienciesWindow, ProficienciesMinusButton, "ProficienciesIndex", i + 9)

	ProficienciesTextArea = GemRB.GetControl(ProficienciesWindow, 68)
	GemRB.SetText(ProficienciesWindow, ProficienciesTextArea, 9588)

	ProficienciesDoneButton = GemRB.GetControl(ProficienciesWindow, 0)
	GemRB.SetButtonState(ProficienciesWindow, ProficienciesDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(ProficienciesWindow, ProficienciesDoneButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesDonePress")
	GemRB.SetText(ProficienciesWindow, ProficienciesDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, ProficienciesDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	ProficienciesCancelButton = GemRB.GetControl(ProficienciesWindow, 77)
	GemRB.SetButtonState(ProficienciesWindow, ProficienciesCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(ProficienciesWindow, ProficienciesCancelButton, IE_GUI_BUTTON_ON_PRESS, "ProficienciesCancelPress")
	GemRB.SetText(ProficienciesWindow, ProficienciesCancelButton, 13727)

	GemRB.SetVisible(ProficienciesWindow, 1)
	return

def ProficienciesLabelPress():
	global ProficienciesWindow, ProficienciesTable, ProficienciesTextArea
	ProficienciesIndex = GemRB.GetVar("ProficienciesIndex") - 1
	GemRB.SetText(ProficienciesWindow, ProficienciesTextArea, GemRB.GetTableValue(ProficienciesTable, ProficienciesIndex, 1) )
	return

def ProficienciesPlusPress():
	global ProficienciesWindow, ProficienciesTable, ProficienciesTextArea
	ProficienciesIndex = GemRB.GetVar("ProficienciesIndex") - 1
	GemRB.SetText(ProficienciesWindow, ProficienciesTextArea, GemRB.GetTableValue(ProficienciesTable, ProficienciesIndex, 1) )
	return

def ProficienciesMinusPress():
	global ProficienciesWindow, ProficienciesTable, ProficienciesTextArea
	ProficienciesIndex = GemRB.GetVar("ProficienciesIndex") - 1
	GemRB.SetText(ProficienciesWindow, ProficienciesTextArea, GemRB.GetTableValue(ProficienciesTable, ProficienciesIndex, 1) )
	return

def ProficienciesDonePress():
	return

def ProficienciesCancelPress():
	global CharGenWindow, ProficienciesWindow
	GemRB.UnloadWindow(ProficienciesWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return

def SkillsSelect():
	global CharGenWindow, SkillsWindow, SkillsTextArea, SkillsDoneButton, RaceTable, ClassTable
	GemRB.SetVisible(CharGenWindow, 0)
	SkillsWindow = GemRB.LoadWindow(6)
	RaceName = GemRB.GetTableRowName(RaceTable, GemRB.GetVar("Race") - 1)
	ClassName = GemRB.GetTableRowName(ClassTable, GemRB.GetVar("Class") - 1)

	for i in range (0, 4):
		SkillsLabelButton = GemRB.GetControl(SkillsWindow, 21 + i)
		GemRB.SetButtonState(SkillsWindow, SkillsLabelButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(SkillsWindow, SkillsLabelButton, IE_GUI_BUTTON_ON_PRESS, "SkillsLabelPress")
		GemRB.SetVarAssoc(SkillsWindow, SkillsLabelButton, "SkillIndex", i + 1)

		SkillsPlusButton = GemRB.GetControl(SkillsWindow, 11 + i * 2)
		GemRB.SetButtonState(SkillsWindow, SkillsPlusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(SkillsWindow, SkillsPlusButton, IE_GUI_BUTTON_ON_PRESS, "SkillsPlusPress")
		GemRB.SetVarAssoc(SkillsWindow, SkillsPlusButton, "SkillIndex", i + 1)

		SkillsMinusButton = GemRB.GetControl(SkillsWindow, 12 + i * 2)
		GemRB.SetButtonState(SkillsWindow, SkillsMinusButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(SkillsWindow, SkillsMinusButton, IE_GUI_BUTTON_ON_PRESS, "SkillsMinusPress")
		GemRB.SetVarAssoc(SkillsWindow, SkillsMinusButton, "SkillIndex", i + 1)

	SkillsTextArea = GemRB.GetControl(SkillsWindow, 19)
	GemRB.SetText(SkillsWindow, SkillsTextArea, 17248)

	SkillsDoneButton = GemRB.GetControl(SkillsWindow, 0)
	GemRB.SetButtonState(SkillsWindow, SkillsDoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(SkillsWindow, SkillsDoneButton, IE_GUI_BUTTON_ON_PRESS, "SkillsDonePress")
	GemRB.SetText(SkillsWindow, SkillsDoneButton, 11973)
	GemRB.SetButtonFlags(GenderWindow, SkillsDoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	SkillsCancelButton = GemRB.GetControl(SkillsWindow, 25)
	GemRB.SetButtonState(SkillsWindow, SkillsCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(SkillsWindow, SkillsCancelButton, IE_GUI_BUTTON_ON_PRESS, "SkillsCancelPress")
	GemRB.SetText(SkillsWindow, SkillsCancelButton, 13727)

	GemRB.SetVisible(SkillsWindow, 1)
	return

def SkillsLabelPress():
	return

def SkillsPlusPress():
	return

def SkillsMinusPress():
	return

def SkillsDonePress():
	return

def SkillsCancelPress():
	global CharGenWindow, SkillsWindow
	GemRB.UnloadWindow(SkillsWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Appearance Selection

def AppearancePress():
	global CharGenWindow, AppearnceWindow
	return

def AppearanceCancelPress():
	global CharGenWindow, AppearanceWindow
	GemRB.UnloadWindow(AppearanceWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Biography Selection

def BiographyPress():
	global CharGenWindow, BiographyWindow
	return

def BiographyCancelPress():
	global CharGenWindow, BiographyWindow
	GemRB.UnloadWindow(BiographyWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Name Selection

def NamePress():
	global CharGenWindow, NameWindow
	return

def NameCancelPress():
	global CharGenWindow, NameWindow
	GemRB.UnloadWindow(NameWindow)
	GemRB.SetVisible(CharGenWindow, 1)
	return


# Import Character

def ImportPress():
	global CharGenWindow
	return

def CancelPress():
	global CharGenWindow
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.SetNextScript("PartyFormation")
	return

def AcceptPress():
	return


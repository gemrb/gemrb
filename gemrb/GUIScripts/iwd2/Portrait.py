#character generation, appearance (GUICG12)
import GemRB

AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0
Gender = 0

def SetPicture():
	global PortraitsTable, LastPortrait

	PortraitName = GemRB.GetTableRowName(PortraitsTable, LastPortrait)+"L"
	GemRB.SetButtonPicture(AppearanceWindow, PortraitButton, PortraitName)
	return

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	global Gender
	
	Gender=GemRB.GetVar("Gender")

	GemRB.LoadWindowPack("GUICG", 800, 600)
	AppearanceWindow = GemRB.LoadWindow(11)
	#GemRB.SetWindowFrame( AppearanceWindow)

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTable("PICTURES")
	LastPortrait = 0

	PortraitButton = GemRB.GetControl(AppearanceWindow, 1)
	GemRB.SetButtonFlags(AppearanceWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	LeftButton = GemRB.GetControl(AppearanceWindow,2)
	RightButton = GemRB.GetControl(AppearanceWindow,3)

	BackButton = GemRB.GetControl(AppearanceWindow,5)
	GemRB.SetText(AppearanceWindow,BackButton,15416)

	CustomButton = GemRB.GetControl(AppearanceWindow, 6)
	GemRB.SetText(AppearanceWindow, CustomButton, 17545)

	DoneButton = GemRB.GetControl(AppearanceWindow,0)
	GemRB.SetText(AppearanceWindow,DoneButton,36789)
	GemRB.SetButtonFlags(AppearanceWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	GemRB.SetEvent(AppearanceWindow,RightButton,IE_GUI_BUTTON_ON_PRESS,"RightPress")
	GemRB.SetEvent(AppearanceWindow,LeftButton,IE_GUI_BUTTON_ON_PRESS,"LeftPress")
	GemRB.SetEvent(AppearanceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(AppearanceWindow,CustomButton,IE_GUI_BUTTON_ON_PRESS,"CustomPress")
	GemRB.SetEvent(AppearanceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	
	while True:
		if GemRB.GetTableValue(PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture()
			break
		LastPortrait = LastPortrait + 1
	GemRB.SetVisible(AppearanceWindow,1)
	return

def RightPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait + 1
		if LastPortrait >= GemRB.GetTableRowCount(PortraitsTable):
			LastPortrait = 0
		if GemRB.GetTableValue(PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture()
			return

def LeftPress():
	global LastPortrait
	while True:
		LastPortrait = LastPortrait - 1
		if LastPortrait < 0:
			LastPortrait = GemRB.GetTableRowCount(PortraitsTable)-1
		if GemRB.GetTableValue(PortraitsTable, LastPortrait, 0) == Gender:
			SetPicture()
			return

def BackPress():
	GemRB.UnloadWindow(AppearanceWindow)
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def CustomPress():
#
	return

def NextPress():
	GemRB.UnloadWindow(AppearanceWindow)
	GemRB.SetVar("PortraitIndex",LastPortrait)
	GemRB.SetNextScript("CharGen2") #Before race
	return


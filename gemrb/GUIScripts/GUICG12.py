#character generation, appearance (GUICG12)
import GemRB

AppearanceWindow = 0
PortraitButton = 0
PortraitsTable = 0
LastPortrait = 0

def OnLoad():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
	
	GemRB.LoadWindowPack("GUICG")
	AppearanceWindow = GemRB.LoadWindow(11)

	#Load the Portraits Table
	PortraitsTable = GemRB.LoadTable("PORTRAIT")

	TextAreaControl = GemRB.GetControl(AppearanceWindow, 7)
	GemRB.SetText(AppearanceWindow, TextAreaControl,"") # why is this here?

	PortraitButton = GemRB.GetControl(AppearanceWindow, 1)
        GemRB.SetButtonFlags(AppearanceWindow, PortraitButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	LeftButton = GemRB.GetControl(AppearanceWindow,2)
	RightButton = GemRB.GetControl(AppearanceWindow,3)

	BackButton = GemRB.GetControl(AppearanceWindow,5)
	GemRB.SetText(AppearanceWindow,BackButton,15416)

	DoneButton = GemRB.GetControl(AppearanceWindow,0)
	GemRB.SetText(AppearanceWindow,DoneButton,11973)

	GemRB.SetEvent(AppearanceWindow,LeftButton,IE_GUI_BUTTON_ON_PRESS,"LeftPress")
	GemRB.SetEvent(AppearanceWindow,RightButton,IE_GUI_BUTTON_ON_PRESS,"RightPress")
	GemRB.SetEvent(AppearanceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AppearanceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	
	PortraitName = GemRB.GetTableRowName(PortraitsTable, LastPortrait)
	print "Loading",PortraitName
	GemRB.SetButtonPicture(AppearanceWindow, PortraitButton, PortraitName)
	
	GemRB.SetVisible(AppearanceWindow,1)
	return

def RightPress():
	global AppearanceWindow, PortraitButton, PortraitsTable, LastPortrait
#Portrait=GetNextPortrait()
#SetPicture(AppearanceWindow, PortraitButton, Portrait)
	LastPortrait = (LastPortrait + 2) % 64
	PortraitName = GemRB.GetTableRowName(PortraitsTable, LastPortrait)
	print "Loading",PortraitName
	GemRB.SetButtonPicture(AppearanceWindow, PortraitButton, PortraitName)
	return

def LeftPress():
#Portrait=GetPrevPortrait()
#SetPicture(AppearanceWindow, PortraitButton, Portrait)
	return

def BackPress():
	GemRB.UnloadWindow(AppearanceWindow)
	GemRB.SetNextScript("GUICG1")
	GemRB.SetVar("Gender",0)  #scrapping the gender value
	return

def NextPress():
        GemRB.UnloadWindow(AppearanceWindow)
	GemRB.SetNextScript("GUICG2") #appearance
	return


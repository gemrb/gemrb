#character generation, race (GUICG2)
import GemRB

CharGenWindow = 0
RaceWindow = 0
TextAreaControl = 0
DoneButton = 0
RaceTable = 0

def OnLoad():
	global CharGenWindow, RaceWindow, TextAreaControl, DoneButton
	global RaceTable
	
	GemRB.LoadWindowPack("GUICG")
        CharGenWindow = GemRB.LoadWindow(0)
	RaceWindow = GemRB.LoadWindow(8)

        for i in range(0,7):
                Button = GemRB.GetControl(CharGenWindow,i)
                GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	RaceTable = GemRB.LoadTable("races")
	RaceCount = GemRB.GetTableRowCount(RaceTable)

	for i in range(2,RaceCount+1):
                Button = GemRB.GetControl(RaceWindow,i)
		GemRB.SetText(RaceWindow,Button, GemRB.GetTableValue(RaceTable,i-2,0) )
                GemRB.SetButtonState(RaceWindow,Button,IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(RaceWindow,Button,IE_GUI_BUTTON_ON_PRESS,"RacePressed")
	for i in range(2, RaceCount+1):
                Button = GemRB.GetControl(RaceWindow,i)
		GemRB.SetVarAssoc(RaceWindow,Button,"Race",i-1)

	PortraitButton = GemRB.GetControl(CharGenWindow,12)
	GemRB.SetButtonFlags(CharGenWindow, PortraitButton, IE_GUI_BUTTON_DISABLED|IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

        DoneButton = GemRB.GetControl(CharGenWindow, 8)
        GemRB.SetText(CharGenWindow, DoneButton, 11962)
        GemRB.SetButtonState(CharGenWindow,DoneButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(CharGenWindow,11)
	GemRB.SetText(CharGenWindow, BackButton, 15416)
	GemRB.SetButtonState(CharGenWindow, BackButton, IE_GUI_BUTTON_DISABLED)

	ImportButton = GemRB.GetControl(CharGenWindow,13)
	GemRB.SetText(CharGenWindow, ImportButton, 13955)
	GemRB.SetButtonState(CharGenWindow, ImportButton, IE_GUI_BUTTON_DISABLED)

	CancelButton = GemRB.GetControl(CharGenWindow, 15)
	GemRB.SetText(CharGenWindow, CancelButton, 8159)
	GemRB.SetButtonState(CharGenWindow,CancelButton,IE_GUI_BUTTON_DISABLED)

	BiographyButton = GemRB.GetControl(CharGenWindow, 16)
	GemRB.SetText(CharGenWindow, BiographyButton, 18003)
	GemRB.SetButtonState(CharGenWindow,BiographyButton,IE_GUI_BUTTON_DISABLED)

	BackButton = GemRB.GetControl(RaceWindow,i+2)  #i=8 now (when race count is 7)
	GemRB.SetText(RaceWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(RaceWindow,0)
	GemRB.SetText(RaceWindow,DoneButton,11973)
        GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_DISABLED)

	TextAreaControl = GemRB.GetControl(RaceWindow, 12)
	GemRB.SetText(RaceWindow,TextAreaControl,17237)

        GemRB.SetEvent(RaceWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
#        GemRB.SetEvent(CharGenWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
        GemRB.SetEvent(RaceWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
        GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(RaceWindow,1)
	return

def RacePressed():
	Race = GemRB.GetVar("Race")-1
	GemRB.SetText(RaceWindow,TextAreaControl, GemRB.GetTableValue(RaceTable,Race,1) )
	GemRB.SetButtonState(RaceWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
#	GemRB.SetButtonState(CharGenWindow,DoneButton,IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("GUICG12")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(RaceWindow)
	GemRB.SetNextScript("CharGen3") #class
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(RaceWindow)
        GemRB.SetNextScript("CharGen")
        return

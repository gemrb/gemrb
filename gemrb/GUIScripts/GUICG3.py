#character generation, alignment (GUICG3)
import GemRB

CharGenWindow = 0
AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global CharGenWindow, AlignmentWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG")
        AlignmentTable = GemRB.LoadTable("aligns")
	CharGenWindow = GemRB.LoadWindow(0)
	AlignmentWindow = GemRB.LoadWindow(3)

	for i in range(0,7):
        	Button = GemRB.GetControl(CharGenWindow,i)
        	GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	for i in range(0,9):
		print i+2
		Button = GemRB.GetControl(AlignmentWindow, i+2)
		GemRB.SetText(AlignmentWindow, Button, GemRB.GetTableValue(AlignmentTable,i,0) )
		GemRB.SetEvent(AlignmentWindow, Button, IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
		GemRB.SetVarAssoc(AlignmentWindow, Button, "Alignment", GemRB.GetTableValue(AlignmentTable,i,3) )

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

	BackButton = GemRB.GetControl(AlignmentWindow,13)
	GemRB.SetText(AlignmentWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AlignmentWindow,0)
	GemRB.SetText(AlignmentWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(AlignmentWindow, 11)
	GemRB.SetText(AlignmentWindow,TextAreaControl,9602)

	GemRB.SetEvent(AlignmentWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AlignmentWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetButtonState(AlignmentWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(AlignmentWindow,1)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")-1
	GemRB.SetText(AlignmentWindow, TextAreaControl, GemRB.GetTableValue(AlignmentTable,Alignment,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetNextScript("CharGen")
	GemRB.SetVar("Alignment",0)  #scrapping the alignment value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetNextScript("GUICG12") #appearance
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(AlignmentWindow)
        GemRB.SetNextScript("CharGen")
        return

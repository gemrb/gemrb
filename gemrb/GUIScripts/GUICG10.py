#character generation, multi-class (GUICG10)
import GemRB

CharGenWindow = 0
ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
ClassTable = 0

def OnLoad():
	global CharGenWindow, ClassWindow, TextAreaControl, DoneButton
	global ClassTable
	
	GemRB.LoadWindowPack("GUICG")
	ClassTable = GemRB.LoadTable("classes")
	ClassCount = GemRB.GetTableRowCount(ClassTable)-1
	CharGenWindow = GemRB.LoadWindow(0)
	ClassWindow = GemRB.LoadWindow(10)

	for i in range(0,7):
        	Button = GemRB.GetControl(CharGenWindow,i)
        	GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	j=0
	for i in range(1,ClassCount):
		if GemRB.GetTableValue(ClassTable,i-1,3)==0:
			continue
		if j>7:
			Button = GemRB.GetControl(ClassWindow,j+7)
		else:
			Button = GemRB.GetControl(ClassWindow,j+2)

		t = GemRB.GetTableValue(ClassTable, i-1, 0)
		GemRB.SetText(ClassWindow, Button, t )
		GemRB.SetEvent(ClassWindow, Button, IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		GemRB.SetVarAssoc(ClassWindow, Button , "Class", i)
		j=j+1

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

	MultiClassButton = GemRB.GetControl(ClassWindow, 10)
	GemRB.SetText(ClassWindow,MultiClassButton, 11993)

	BackButton = GemRB.GetControl(ClassWindow,14)
	GemRB.SetText(ClassWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ClassWindow,0)
	GemRB.SetText(ClassWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(ClassWindow, 13)
	GemRB.SetText(ClassWindow,TextAreaControl,17242)

	GemRB.SetEvent(ClassWindow,MultiClassButton,IE_GUI_BUTTON_ON_PRESS,"MultiClassPress")
	GemRB.SetEvent(ClassWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetButtonState(ClassWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(ClassWindow,1)
	return

def MultiClassPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("GUICG10")
	return

def ClassPress():
	Class = GemRB.GetVar("Class")-1
	GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(CharGenWindow)
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	return

def NextPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen4") #alignment
	return

def CancelPress():
        GemRB.UnloadWindow(CharGenWindow)
        GemRB.UnloadWindow(ClassWindow)
        GemRB.SetNextScript("CharGen")
        return

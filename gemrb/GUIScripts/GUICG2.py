#character generation, class (GUICG2)
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
	ClassWindow = GemRB.LoadWindow(2)

	for i in range(0,7):
        	Button = GemRB.GetControl(CharGenWindow,i)
        	GemRB.SetButtonState(CharGenWindow,Button,IE_GUI_BUTTON_DISABLED)

	j=0
	for i in range(1,ClassCount):
		if GemRB.GetTableValue(ClassTable,i-1,3)==0:
			continue
		if j>7:
			r=j+7
		else:
			r=j+2

		Button = GemRB.GetControl(ClassWindow,r)
		t = GemRB.GetTableValue(ClassTable, i-1, 0)
		print "Control:",r,"Text:",t
		GemRB.SetText(ClassWindow, r, t )
		GemRB.SetEvent(ClassWindow, r, IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		GemRB.SetVarAssoc(ClassWindow, r, "Class", i)
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

	BackButton = GemRB.GetControl(ClassWindow,13)
	GemRB.SetText(ClassWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ClassWindow,0)
	GemRB.SetText(ClassWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(ClassWindow, 11)
	GemRB.SetText(ClassWindow,TextAreaControl,17236)

	MaleButton = GemRB.GetControl(ClassWindow,2)
	GemRB.SetButtonFlags(ClassWindow,MaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	FemaleButton = GemRB.GetControl(ClassWindow,3)
	GemRB.SetButtonFlags(ClassWindow,FemaleButton,IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	GemRB.SetVarAssoc(ClassWindow,MaleButton,"Gender",1)
	GemRB.SetVarAssoc(ClassWindow,FemaleButton,"Gender",2)
	GemRB.SetEvent(ClassWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetEvent(CharGenWindow,CancelButton,IE_GUI_BUTTON_ON_PRESS,"CancelPress")
	GemRB.SetButtonState(ClassWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(CharGenWindow,1)
	GemRB.SetVisible(ClassWindow,1)
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

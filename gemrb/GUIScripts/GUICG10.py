#character generation, multi-class (GUICG10)
import GemRB

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
ClassTable = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton
	global ClassTable
	
	GemRB.LoadWindowPack("GUICG")
	ClassTable = GemRB.LoadTable("classes")
	ClassCount = GemRB.GetTableRowCount(ClassTable)-1
	ClassWindow = GemRB.LoadWindow(10)

	j=0
	for i in range(1,ClassCount):
		if GemRB.GetTableValue(ClassTable,i-1,3)==0:
			continue
		if j>11:
			Button = GemRB.GetControl(ClassWindow,j+7)
		else:
			Button = GemRB.GetControl(ClassWindow,j+2)
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(ClassWindow, Button, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)

	j=0
	for i in range(1,ClassCount):
		if GemRB.GetTableValue(ClassTable,i-1,3)==0:
			continue
		if j>11:
			Button = GemRB.GetControl(ClassWindow,j+7)
		else:
			Button = GemRB.GetControl(ClassWindow,j+2)

		t = GemRB.GetTableValue(ClassTable, i-1, 0)
		GemRB.SetText(ClassWindow, Button, t )
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ClassWindow, Button, IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		GemRB.SetVarAssoc(ClassWindow, Button , "Class", i) #multiclass, actually
		j=j+1

	BackButton = GemRB.GetControl(ClassWindow,14)
	GemRB.SetText(ClassWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ClassWindow,0)
	GemRB.SetText(ClassWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(ClassWindow, 12)
	GemRB.SetText(ClassWindow,TextAreaControl,17244)

	GemRB.SetEvent(ClassWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(ClassWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(ClassWindow,1)
	return

def ClassPress():
	Class = GemRB.GetVar("Class")-1
	GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
        GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen4") #alignment
	return

#character generation, class (GUICG2)
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
	ClassCount = GemRB.GetTableRowCount(ClassTable)+1
	ClassWindow = GemRB.LoadWindow(2)
	TmpTable=GemRB.LoadTable("races")
        RaceName = GemRB.GetTableRowName(TmpTable, GemRB.GetVar("Race")-1 )

	j = 0
	#radiobutton groups must be set up before doing anything else to them
	for i in range(1,ClassCount):
		Button = GemRB.GetControl(ClassWindow,j+2)
		GemRB.SetButtonFlags(ClassWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_DISABLED)
		j = j+1

	j = 0
	GemRB.SetVar("MAGESCHOOL",0) 
	HasMulti = 0
	for i in range(1,ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, RaceName)
		Button = GemRB.GetControl(ClassWindow,j+2)
		j = j+1
		t = GemRB.GetTableValue(ClassTable, i-1, 0)
		GemRB.SetText(ClassWindow, Button, t )

		if Allowed==0:
			continue
		if Allowed==2:
			GemRB.SetVar("MAGESCHOOL",5) #illusionist
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ClassWindow, Button, IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		GemRB.SetVarAssoc(ClassWindow, Button , "Class", i)

	BackButton = GemRB.GetControl(ClassWindow,0)
	GemRB.SetText(ClassWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ClassWindow,17)
	GemRB.SetText(ClassWindow,DoneButton,11973)

	ScrollBarControl = GemRB.GetControl(ClassWindow, 15)

	TextAreaControl = GemRB.GetControl(ClassWindow, 16)

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		GemRB.SetText(ClassWindow,TextAreaControl,17242)
		GemRB.SetButtonState(ClassWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	else:
		GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
		GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)

	GemRB.SetEvent(ClassWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(ClassWindow,1)
	return

def ClassPress():
	Class = GemRB.GetVar("Class")-1
	GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	#if no kit selection for this class, don't go to guicg22
	GemRB.DrawWindows()
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("GUICG22")
	return

def BackPress():
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	return

def NextPress():
        GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen4") #alignment
	return

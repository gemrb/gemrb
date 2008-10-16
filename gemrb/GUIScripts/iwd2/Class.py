#character generation, class (GUICG2)
import GemRB

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
BackButton = 0
ClassTable = 0
ClassCount = 0
HasSubClass = 0
ClassID = 0

def AdjustTextArea():
	global HasSubClass, ClassID

	Class = GemRB.GetVar("Class")-1
	GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
	ClassName = GemRB.GetTableRowName(ClassTable, Class)
	ClassID = GemRB.GetTableValue(ClassTable, ClassName, "ID")
	#determining if this class has any subclasses
	HasSubClass = 0
	for i in range(1, ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		#determining if this is a kit or class
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		HasSubClass = 1
		break

	if HasSubClass == 0:
		GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	else:
		GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, BackButton
	global ClassTable, ClassCount
	
	GemRB.LoadWindowPack("GUICG", 800, 600)
	#this replaces help02.2da for class restrictions
	ClassTable = GemRB.LoadTable("classes")
	ClassCount = GemRB.GetTableRowCount(ClassTable)+1
	ClassWindow = GemRB.LoadWindow(2)
	TmpTable=GemRB.LoadTable("races")
	rid = GemRB.FindTableValue(TmpTable, 3, GemRB.GetVar('BaseRace'))
	RaceName = GemRB.GetTableRowName(TmpTable, rid)

	#radiobutton groups must be set up before doing anything else to them
	j = 0
	for i in range(1,ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, "CLASS")
		if Allowed > 0:
			continue
		Button = GemRB.GetControl(ClassWindow,j+2)
		j = j+1
		GemRB.SetButtonFlags(ClassWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_DISABLED)

	j = 0
	for i in range(1,ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		#determining if this is a kit or class
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, "CLASS")
		if Allowed > 0:
			continue
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, RaceName)
		Button = GemRB.GetControl(ClassWindow,j+2)
		j = j+1
		t = GemRB.GetTableValue(ClassTable, ClassName, "NAME_REF")
		GemRB.SetText(ClassWindow, Button, t )

		if Allowed==0:
			continue
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ClassWindow, Button, IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		GemRB.SetVarAssoc(ClassWindow, Button , "Class", i)

	BackButton = GemRB.GetControl(ClassWindow,17)
	GemRB.SetText(ClassWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(ClassWindow,0)
	GemRB.SetText(ClassWindow,DoneButton,36789)
	GemRB.SetButtonFlags(ClassWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	ScrollBarControl = GemRB.GetControl(ClassWindow, 15)

	TextAreaControl = GemRB.GetControl(ClassWindow, 16)

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		GemRB.SetText(ClassWindow,TextAreaControl,17242)
		GemRB.SetButtonState(ClassWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	else:
		AdjustTextArea()

	GemRB.SetEvent(ClassWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetVisible(ClassWindow,1)
	return

def ClassPress():
	global HasSubClass

	AdjustTextArea()
	if HasSubClass == 0:
		return

	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	j = 0
	for i in range(1,ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, "CLASS")
		if Allowed > 0:
			continue
		Button = GemRB.GetControl(ClassWindow,j+2)
		j = j+1
		GemRB.SetButtonFlags(ClassWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetText(ClassWindow, Button, "")

	j=0
	for i in range(1, ClassCount):
		ClassName = GemRB.GetTableRowName(ClassTable, i-1)
		#determining if this is a kit or class
		Allowed = GemRB.GetTableValue(ClassTable, ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		Button = GemRB.GetControl(ClassWindow,j+2)
		j = j+1
		t = GemRB.GetTableValue(ClassTable, ClassName, "NAME_REF")
		GemRB.SetText(ClassWindow, Button, t )
		GemRB.SetButtonState(ClassWindow, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetEvent(ClassWindow, Button, IE_GUI_BUTTON_ON_PRESS,  "ClassPress2")
		GemRB.SetVarAssoc(ClassWindow, Button , "Class", i)
		
	GemRB.SetEvent(ClassWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress2")
	return

def ClassPress2():
	Class = GemRB.GetVar("Class")-1
	GemRB.SetText(ClassWindow,TextAreaControl, GemRB.GetTableValue(ClassTable,Class,1) )
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress2():
	GemRB.SetButtonState(ClassWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.UnloadWindow(ClassWindow)
	OnLoad()
	return

def BackPress():
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
        MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (IE_CLASS, 0)
	return

def NextPress():
	#classcolumn is base class
	Class = GemRB.GetVar("Class")
	ClassColumn = GemRB.GetTableValue(ClassTable, Class - 1, 3)
	if ClassColumn <= 0:  #it was already a base class
		ClassColumn = Class 
	GemRB.SetVar("BaseClass", ClassColumn)
	GemRB.UnloadWindow(ClassWindow)
	GemRB.SetNextScript("CharGen4") #alignment
	return

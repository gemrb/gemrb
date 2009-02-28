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
	TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
	ClassName = ClassTable.GetRowName(Class)
	ClassID = ClassTable.GetValue(ClassName, "ID")
	#determining if this class has any subclasses
	HasSubClass = 0
	for i in range(1, ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = ClassTable.GetValue(ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		HasSubClass = 1
		break

	if HasSubClass == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, BackButton
	global ClassTable, ClassCount

	GemRB.LoadWindowPack("GUICG", 800, 600)
	#this replaces help02.2da for class restrictions
	ClassTable = GemRB.LoadTableObject("classes")
	ClassCount = ClassTable.GetRowCount()+1
	ClassWindow = GemRB.LoadWindowObject(2)
	TmpTable=GemRB.LoadTableObject("races")
	rid = TmpTable.FindValue(3, GemRB.GetVar('BaseRace'))
	RaceName = TmpTable.GetRowName(rid)

	#radiobutton groups must be set up before doing anything else to them
	j = 0
	for i in range(1,ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, "CLASS")
		if Allowed > 0:
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)

	j = 0
	for i in range(1,ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = ClassTable.GetValue(ClassName, "CLASS")
		if Allowed > 0:
			continue
		Allowed = ClassTable.GetValue(ClassName, RaceName)
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = ClassTable.GetValue(ClassName, "NAME_REF")
		Button.SetText(t )

		if Allowed==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		Button.SetVarAssoc("Class", i)

	BackButton = ClassWindow.GetControl(17)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	ScrollBarControl = ClassWindow.GetControl(15)

	TextAreaControl = ClassWindow.GetControl(16)

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		AdjustTextArea()

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	ClassWindow.SetVisible(1)
	return

def ClassPress():
	global HasSubClass

	AdjustTextArea()
	if HasSubClass == 0:
		return

	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	j = 0
	for i in range(1,ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, "CLASS")
		if Allowed > 0:
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText("")

	j=0
	for i in range(1, ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = ClassTable.GetValue(ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = ClassTable.GetValue(ClassName, "NAME_REF")
		Button.SetText(t )
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress2")
		Button.SetVarAssoc("Class", i)

	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress2")
	return

def ClassPress2():
	Class = GemRB.GetVar("Class")-1
	TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress2():
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	if ClassWindow:
		ClassWindow.Unload()
	OnLoad()
	return

def BackPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (IE_CLASS, 0)
	return

def NextPress():
	#classcolumn is base class
	Class = GemRB.GetVar("Class")
	ClassColumn = ClassTable.GetValue(Class - 1, 3)
	if ClassColumn <= 0:  #it was already a base class
		ClassColumn = Class 
	GemRB.SetVar("BaseClass", ClassColumn)
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen4") #alignment
	return

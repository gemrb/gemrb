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
	ClassTable = GemRB.LoadTableObject("classes")
	ClassCount = ClassTable.GetRowCount()+1
	ClassWindow = GemRB.LoadWindowObject(10)
	TmpTable=GemRB.LoadTableObject("races")
	RaceName = TmpTable.GetRowName(GemRB.GetVar("Race")-1 )

	j=0
	for i in range(1,ClassCount):
		if ClassTable.GetValue(i-1,4)==0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		j = j + 1
	j=0
	for i in range(1,ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, RaceName)
		if ClassTable.GetValue(i-1,4)==0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)

		t = ClassTable.GetValue(i-1, 0)
		Button.SetText(t )
		j=j+1
		if Allowed ==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		Button.SetVarAssoc("Class", i) #multiclass, actually

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	ClassWindow.SetVisible(1)
	return

def ClassPress():
	Class = GemRB.GetVar("Class")-1
	TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.SetVar("Class",0)  # scrapping it
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen4") #alignment
	return

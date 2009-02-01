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
	ClassTable = GemRB.LoadTableObject("classes")
	ClassCount = ClassTable.GetRowCount()+1
	ClassWindow = GemRB.LoadWindowObject(2)
	TmpTable=GemRB.LoadTableObject("races")
	RaceName = TmpTable.GetRowName(GemRB.GetVar("Race")-1 )

	#radiobutton groups must be set up before doing anything else to them
	for i in range(1,ClassCount):
		if ClassTable.GetValue(i-1,4):
			continue
			
		Button = ClassWindow.GetControl(i+1)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)

	GemRB.SetVar("MAGESCHOOL",0) 
	HasMulti = 0
	for i in range(1,ClassCount):
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, RaceName)
		if ClassTable.GetValue(i-1,4):
			if Allowed!=0:
				HasMulti = 1
			continue
			
		Button = ClassWindow.GetControl(i+1)
		
		t = ClassTable.GetValue(i-1, 0)
		Button.SetText(t )

		if Allowed==2:
			GemRB.SetVar("MAGESCHOOL",5) #illusionist
		if Allowed!=1:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		Button.SetVarAssoc("Class", i)

	MultiClassButton = ClassWindow.GetControl(10)
	MultiClassButton.SetText(11993)
	if HasMulti == 0:
		MultiClassButton.SetState(IE_GUI_BUTTON_DISABLED)

	SpecialistButton = ClassWindow.GetControl(11)
	SpecialistButton.SetText(11994)
	
	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = ClassWindow.GetControl(13)

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	MultiClassButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"MultiClassPress")
	SpecialistButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SpecialistPress")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	ClassWindow.SetVisible(1)
	return

def MultiClassPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetVar("Class Kit",0)
	GemRB.SetNextScript("GUICG10")
	return

def SpecialistPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetVar("Class Kit", 0)
	GemRB.SetVar("Class", 6)
	GemRB.SetNextScript("GUICG22")
	return
	
def ClassPress():
	Class = GemRB.GetVar("Class")-1
	TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	return

def NextPress():
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("CharGen4") #alignment
	return

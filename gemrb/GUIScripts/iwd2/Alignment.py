#character generation, alignment (GUICG3)
import GemRB

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
AlignmentTable = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global AlignmentTable
	
	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetVar("Class")-1
	KitName = ClassTable.GetRowName(Class)

	AlignmentOk = GemRB.LoadTableObject("ALIGNMNT")

	GemRB.LoadWindowPack("GUICG", 800, 600)
	AlignmentTable = GemRB.LoadTableObject("aligns")
	AlignmentWindow = GemRB.LoadWindowObject(3)
	for i in range(0,9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText(AlignmentTable.GetValue(i,0) )

	for i in range(0,9):
		Button = AlignmentWindow.GetControl(i+2)
		if AlignmentOk.GetValue(KitName, AlignmentTable.GetValue(i, 4) ) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
			Button.SetVarAssoc("Alignment", i+1)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.SetVisible(1)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")-1
	TextAreaControl.SetText(AlignmentTable.GetValue(Alignment,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	return

def NextPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetNextScript("CharGen5") #appearance
	return

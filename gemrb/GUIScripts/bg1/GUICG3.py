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

	GemRB.LoadWindowPack("GUICG")
	AlignmentTable = GemRB.LoadTableObject("aligns")
	AlignmentWindow = GemRB.LoadWindowObject(3)

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText(AlignmentTable.GetValue(i,0) )

	# This section enables or disables different alignment selections
	# based on Class, and depends on the ALIGNMNT.2DA table
	#
	# For now, we just enable all buttons
	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		if AlignmentOk.GetValue(KitName, AlignmentTable.GetValue(i, 4) ) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
		Button.SetVarAssoc("Alignment", i)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)


	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.SetVisible(1)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")
	TextAreaControl.SetText(AlignmentTable.GetValue(Alignment,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	GemRB.SetVar("Alignment",AlignmentTable.GetValue(Alignment,3) )
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

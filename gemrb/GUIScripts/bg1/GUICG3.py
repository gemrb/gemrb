#character generation, alignment (GUICG3)
import GemRB

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
AlignmentTable = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global AlignmentTable
	
	ClassTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	KitName = GemRB.GetTableRowName(ClassTable, Class)

        AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	GemRB.LoadWindowPack("GUICG")
	AlignmentTable = GemRB.LoadTable("aligns")
	AlignmentWindow = GemRB.LoadWindow(3)

	for i in range(0,9):
		Button = GemRB.GetControl(AlignmentWindow, i+2)
		GemRB.SetButtonFlags(AlignmentWindow, Button, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		GemRB.SetButtonState(AlignmentWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetText(AlignmentWindow, Button, GemRB.GetTableValue(AlignmentTable,i,0) )

	# This section enables or disables different alignment selections
	# based on Class, and depends on the ALIGNMNT.2DA table
	#
	# For now, we just enable all buttons
	for i in range(0,9):
		Button = GemRB.GetControl(AlignmentWindow, i+2)
		if GemRB.GetTableValue(AlignmentOk, KitName, GemRB.GetTableValue(AlignmentTable, i, 4) ) != 0:
			GemRB.SetButtonState(AlignmentWindow, Button, IE_GUI_BUTTON_ENABLED)
		else:
			GemRB.SetButtonState(AlignmentWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetEvent(AlignmentWindow, Button, IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
		GemRB.SetVarAssoc(AlignmentWindow, Button, "Alignment", i)

	BackButton = GemRB.GetControl(AlignmentWindow,13)
	GemRB.SetText(AlignmentWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AlignmentWindow,0)
	GemRB.SetText(AlignmentWindow,DoneButton,11973)
	GemRB.SetButtonFlags(AlignmentWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)


	TextAreaControl = GemRB.GetControl(AlignmentWindow, 11)
	GemRB.SetText(AlignmentWindow,TextAreaControl,9602)

	GemRB.SetEvent(AlignmentWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(AlignmentWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(AlignmentWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	GemRB.SetVisible(AlignmentWindow,1)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")
	GemRB.SetText(AlignmentWindow, TextAreaControl, GemRB.GetTableValue(AlignmentTable,Alignment,1) )
	GemRB.SetButtonState(AlignmentWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetVar("Alignment",GemRB.GetTableValue(AlignmentTable,Alignment,3) )
	return

def BackPress():
	GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	return

def NextPress():
	GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetNextScript("CharGen5") #appearance
	return

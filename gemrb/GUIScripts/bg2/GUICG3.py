#character generation, alignment (GUICG3)
import GemRB

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
AlignmentTable = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global AlignmentTable
	
	GemRB.LoadWindowPack("GUICG")
        AlignmentTable = GemRB.LoadTable("aligns")
	AlignmentWindow = GemRB.LoadWindow(3)

	for i in range(0,9):
		Button = GemRB.GetControl(AlignmentWindow, i+2)
		GemRB.SetButtonFlags(AlignmentWindow, Button, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	for i in range(0,9):
		Button = GemRB.GetControl(AlignmentWindow, i+2)
		GemRB.SetText(AlignmentWindow, Button, GemRB.GetTableValue(AlignmentTable,i,0) )
		GemRB.SetEvent(AlignmentWindow, Button, IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
		GemRB.SetVarAssoc(AlignmentWindow, Button, "Alignment", i)

	BackButton = GemRB.GetControl(AlignmentWindow,13)
	GemRB.SetText(AlignmentWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(AlignmentWindow,0)
	GemRB.SetText(AlignmentWindow,DoneButton,11973)

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
	GemRB.SetVar("Alignment",0)  #scrapping the alignment value
	return

def NextPress():
        GemRB.UnloadWindow(AlignmentWindow)
	GemRB.SetNextScript("CharGen5") #appearance
	return

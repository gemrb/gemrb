#character generation, class kit (GUICG22)
import GemRB

KitWindow = 0
TextAreaControl = 0
DoneButton = 0
KitList = 0
ClassList = 0

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global KitList, ClassList
	
	GemRB.LoadWindowPack("GUICG")
	TmpTable = GemRB.LoadTable("races")
	RaceName = GemRB.GetTableRowName(TmpTable, GemRB.GetVar("Race")-1 )
	TmpTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	ClassName = GemRB.GetTableRowName(TmpTable, Class)
	ClassList = GemRB.LoadTable("classes")
        KitTable = GemRB.LoadTable("kittable")
	KitTableName = GemRB.GetTableValue(KitTable, ClassName, RaceName)
	print "Kittable: ",KitTableName
	KitTable = GemRB.LoadTable(KitTableName)
	KitList = GemRB.LoadTable("kitlist")

	KitWindow = GemRB.LoadWindow(22)
	if Class == 0:
		Label = GemRB.GetControl(KitWindow, 0xfffffff)
		GemRB.SetText(KitWindow, Label, 595)

	for i in range(0,10):
		if i<4:
			Button = i+1
		else:
			Button = i+5
		GemRB.SetButtonState(KitWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(KitWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range(0,GemRB.GetTableRowCount(KitTable)-1 ):
		if i<4:
			Button = i+1
		else:
			Button = i+5
		GemRB.SetButtonState(KitWindow, Button, IE_GUI_BUTTON_ENABLED)
		Kit = GemRB.GetTableValue(KitTable,i,0)
		if Kit == 0:
			KitName = GemRB.GetTableValue(ClassList, GemRB.GetVar("Class")-1, 0)
		else:
			KitName = GemRB.GetTableValue(KitList, Kit, 1)
		GemRB.SetText(KitWindow, Button, KitName)
		GemRB.SetVarAssoc(KitWindow, Button, "Class Kit",Kit)
		GemRB.SetEvent(KitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "KitPress")

	BackButton = GemRB.GetControl(KitWindow,8)
	GemRB.SetText(KitWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(KitWindow,7)
	GemRB.SetText(KitWindow,DoneButton,11973)

	TextAreaControl = GemRB.GetControl(KitWindow, 5)
	GemRB.SetText(KitWindow,TextAreaControl,17247)

	GemRB.SetEvent(KitWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(KitWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	KitPress()
	GemRB.SetVisible(KitWindow,1)
	return

def KitPress():
	Kit = GemRB.GetVar("Class Kit")
	if Kit == 0:
		KitName = GemRB.GetTableValue(ClassList, GemRB.GetVar("Class")-1, 1)
	else:
		KitName = GemRB.GetTableValue(KitList, Kit, 3)
	GemRB.SetText(KitWindow, TextAreaControl, KitName)
	GemRB.SetButtonState(KitWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.SetVar("Class Kit",0) #scrapping
	GemRB.UnloadWindow(KitWindow)
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
        GemRB.UnloadWindow(KitWindow)
	GemRB.SetNextScript("CharGen4") #abilities
	return

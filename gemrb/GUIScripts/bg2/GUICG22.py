#character generation, class kit (GUICG22)
import GemRB

KitWindow = 0
TextAreaControl = 0
DoneButton = 0
KitList = 0
ClassList = 0
SchoolList = 0
ClassID = 0

def OnLoad():
	global KitWindow, TextAreaControl, DoneButton
	global KitList, ClassList, SchoolList, ClassID
	
	GemRB.LoadWindowPack("GUICG")
	TmpTable = GemRB.LoadTable("races")
	RaceName = GemRB.GetTableRowName(TmpTable, GemRB.GetVar("Race")-1 )
	TmpTable = GemRB.LoadTable("classes")
	Class = GemRB.GetVar("Class")-1
	ClassName = GemRB.GetTableRowName(TmpTable, Class)
	ClassID = GemRB.GetTableValue(TmpTable, Class, 5)
	ClassList = GemRB.LoadTable("classes")
	KitTable = GemRB.LoadTable("kittable")
	KitTableName = GemRB.GetTableValue(KitTable, ClassName, RaceName)
	KitTable = GemRB.LoadTable(KitTableName,1)

	KitList = GemRB.LoadTable("kitlist")
	SchoolList = GemRB.LoadTable("magesch")

	#there is a specialist mage window, but it is easier to use 
	#the class kit window for both
	KitWindow = GemRB.LoadWindow(22)
	if ClassID == 1:
		Label = GemRB.GetControl(KitWindow, 0xfffffff)
		GemRB.SetText(KitWindow, Label, 595)

	for i in range(10):
		if i<4:
			Button = GemRB.GetControl(KitWindow, i+1)
		else:
			Button = GemRB.GetControl(KitWindow, i+5)
		GemRB.SetButtonState(KitWindow, Button, IE_GUI_BUTTON_DISABLED)
		GemRB.SetButtonFlags(KitWindow, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	if KitTable == -1:
		RowCount = 1
	else:
		RowCount = GemRB.GetTableRowCount(KitTable)

	for i in range(RowCount):
		if i<4:
			Button = GemRB.GetControl(KitWindow, i+1)
		else:
			Button = GemRB.GetControl(KitWindow, i+5)
		if KitTable == -1:
			if ClassID == 1:
				Kit=GemRB.GetVar("MAGESCHOOL")
				KitName = GemRB.GetTableValue(SchoolList, i, 0)
			else:
				Kit = 0
				KitName = GemRB.GetTableValue(ClassList, GemRB.GetVar("Class")-1, 0)

		else:
			Kit = GemRB.GetTableValue(KitTable,i,0)
			if ClassID == 1:
				if Kit:
					Kit = Kit - 21
				KitName = GemRB.GetTableValue(SchoolList, Kit, 0)
			else:
				if Kit:
					KitName = GemRB.GetTableValue(KitList, Kit, 1)
				else:
					KitName = GemRB.GetTableValue(ClassList, GemRB.GetVar("Class")-1, 0)

		GemRB.SetButtonState(KitWindow, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetText(KitWindow, Button, KitName)
		GemRB.SetVarAssoc(KitWindow, Button, "Class Kit",Kit)
		if i==0:
			GemRB.SetVar("Class Kit",Kit)
		GemRB.SetEvent(KitWindow, Button, IE_GUI_BUTTON_ON_PRESS, "KitPress")

	BackButton = GemRB.GetControl(KitWindow,8)
	GemRB.SetText(KitWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(KitWindow,7)
	GemRB.SetText(KitWindow,DoneButton,11973)
	GemRB.SetButtonFlags(KitWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

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
		if ClassID==1:
			KitName = GemRB.GetTableValue(SchoolList, Kit, 1)
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

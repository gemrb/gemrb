#character generation, skills (GUICG6)
import GemRB

FeatWindow = 0
TextAreaControl = 0
DoneButton = 0
FeatTable = 0
TopIndex = 0
Level = 0
ClassColumn = 0
PointsLeft = 0

def RedrawFeats():
	global TopIndex

	SumLabel = GemRB.GetControl(FeatWindow, 0x1000000c)
	GemRB.SetText(FeatWindow, SumLabel, str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i+1
		FeatName = GemRB.GetTableValue(FeatTable, Pos, 1)
		Label = GemRB.GetControl(FeatWindow, 0x10000001+i)
		GemRB.SetText(FeatWindow, Label, FeatName)

		FeatName=GemRB.GetTableRowName(FeatTable, Pos) #row name
		
		Button1 = GemRB.GetControl(FeatWindow, i*2+14)
		Button2 = GemRB.GetControl(FeatWindow, i*2+15)
		Ok = 1
		if Ok == 0:
			GemRB.SetButtonState(FeatWindow, Button1, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(FeatWindow, Button2, IE_GUI_BUTTON_DISABLED)
#			GemRB.SetButtonFlags(FeatWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
#			GemRB.SetButtonFlags(FeatWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_OR)
		else:
			GemRB.SetButtonState(FeatWindow, Button1, IE_GUI_BUTTON_ENABLED)
			GemRB.SetButtonState(FeatWindow, Button2, IE_GUI_BUTTON_ENABLED)
#			GemRB.SetButtonFlags(FeatWindow, Button1, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)
#			GemRB.SetButtonFlags(FeatWindow, Button2, IE_GUI_BUTTON_NO_IMAGE,OP_NAND)

	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawFeats()
	return

def OnLoad():
	global FeatWindow, TextAreaControl, DoneButton, TopIndex
	global FeatTable, CostTable
	global KitName, Level, ClassColumn
	
	GemRB.SetVar("Level",1) #for simplicity
	Class = GemRB.GetVar("Class") - 1
	ClassTable = GemRB.LoadTable("classes")
	KitName = GemRB.GetTableRowName(ClassTable, Class)
	#classcolumn is base class
	ClassColumn = GemRB.GetTableValue(ClassTable, Class, 3) - 1
	if ClassColumn < 0:  #it was already a base class
		ClassColumn = Class

	FeatTable = GemRB.LoadTable("feats")
	RowCount = GemRB.GetTableRowCount(FeatTable)

	for i in range(0,RowCount):
			GemRB.SetVar("Feat "+str(i),0)

	FeatClassTable = GemRB.LoadTable("featclas")
	#calculating the number of new feats
	for l in (1,Level+1):
		PointsLeft = GemRB.GetTableValue(FeatTable, l, ClassColumn)
		print "Pl:", PointsLeft
		print "a", GemRB.GetTableValue(FeatTable, l, ClassColumn)
		print "b", GemRB.GetTableValue(FeatClassTable, l, ClassColumn)
		PointsLeft += GemRB.GetTableValue(FeatClassTable, l, ClassColumn)
	
	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG")
	FeatWindow = GemRB.LoadWindow(55)

	for i in range(0,10):
		Button = GemRB.GetControl(FeatWindow, i+93)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button = GemRB.GetControl(FeatWindow, i*2+14)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button = GemRB.GetControl(FeatWindow, i*2+15)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")

	BackButton = GemRB.GetControl(FeatWindow,105)
	GemRB.SetText(FeatWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(FeatWindow,0)
	GemRB.SetText(FeatWindow,DoneButton,36789)
	GemRB.SetButtonFlags(FeatWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(FeatWindow, 92)
	GemRB.SetText(FeatWindow,TextAreaControl,17248)

	ScrollBarControl = GemRB.GetControl(FeatWindow, 104)
	GemRB.SetEvent(FeatWindow, ScrollBarControl,IE_GUI_SCROLLBAR_ON_CHANGE,"ScrollBarPress")
	#decrease it with the number of controls on screen (list size)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	GemRB.SetVarAssoc(FeatWindow, ScrollBarControl, "TopIndex",RowCount-10)

	GemRB.SetEvent(FeatWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(FeatWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	GemRB.SetButtonState(FeatWindow,DoneButton,IE_GUI_BUTTON_DISABLED)
	RedrawFeats()
	GemRB.SetVisible(FeatWindow,1)
	return


def JustPress():
	Pos = GemRB.GetVar("Feat")+TopIndex+1
	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex+1
	Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)
	if Cost==0:
		return

	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Feat "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + Cost
	RedrawFeats()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex+1
	Cost = GemRB.GetTableValue(CostTable, Pos, ClassColumn)
	if Cost==0:
		return

	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	if PointsLeft < Cost:
		return
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
	if ActPoint >= Level:
		return
	GemRB.SetVar("Feat "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - Cost
	RedrawFeats()
	return

def BackPress():
	GemRB.UnloadWindow(FeatWindow)
	GemRB.SetNextScript("Skills")
	return

def NextPress():
	GemRB.UnloadWindow(FeatWindow)
	GemRB.SetNextScript("CharGen7")
	return

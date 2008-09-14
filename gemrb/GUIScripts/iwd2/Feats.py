#character generation, skills (GUICG6)
import GemRB

FeatWindow = 0
TextAreaControl = 0
DoneButton = 0
FeatTable = 0
TopIndex = 0
Level = 0
ClassColumn = 0
KitColumn = 0
RaceColumn = 0
PointsLeft = 0

def GetBaseValue(feat):
	Val = GemRB.GetTableValue(FeatTable, feat, ClassColumn)
	Val += GemRB.GetTableValue(FeatTable, feat, RaceColumn)
	if KitColumn != 0:
		Val += GemRB.GetTableValue(FeatTable, feat, KitColumn)
	return Val

def RedrawFeats():
	global TopIndex, PointsLeft

	SumLabel = GemRB.GetControl(FeatWindow, 0x1000000c)
	if PointsLeft == 0:
		GemRB.SetButtonState(FeatWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetLabelTextColor(FeatWindow,SumLabel, 255, 255, 255)
	else:
		GemRB.SetButtonState(FeatWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetLabelTextColor(FeatWindow,SumLabel, 255, 255, 0)

	GemRB.SetText(FeatWindow, SumLabel, str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i+1
		FeatName = GemRB.GetTableValue(FeatTable, Pos, 1)
		Label = GemRB.GetControl(FeatWindow, 0x10000001+i)
		GemRB.SetText(FeatWindow, Label, FeatName)

		FeatName=GemRB.GetTableRowName(FeatTable, Pos) #row name
		FeatValue = GemRB.GetVar("Feat "+str(Pos))

		ButtonPlus = GemRB.GetControl(FeatWindow, i*2+14)
		ButtonMinus = GemRB.GetControl(FeatWindow, i*2+15)
		if FeatValue == 0:
			GemRB.SetButtonState(FeatWindow, ButtonMinus, IE_GUI_BUTTON_DISABLED)
			# TODO: check if feat is usable
			if True:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
		else:
			# TODO: check for maximum if there are more feat levels
			if True:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_ENABLED)
			else:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(FeatWindow, ButtonMinus, IE_GUI_BUTTON_ENABLED)

		if PointsLeft == 0:
			GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
			GemRB.SetLabelTextColor(FeatWindow, Label, 150, 150, 150)
		else:
			GemRB.SetLabelTextColor(FeatWindow, Label, 255, 255, 255)
	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawFeats()
	return

def OnLoad():
	global FeatWindow, TextAreaControl, DoneButton, TopIndex
	global FeatTable
	global KitName, Level, PointsLeft
	global ClassColumn, KitColumn, RaceColumn
	
	GemRB.SetVar("Level",1) #for simplicity

	Race = GemRB.GetVar("Race")
	RaceTable = GemRB.LoadTable("races")
	RaceColumn = GemRB.FindTableValue(RaceTable, 3, Race)
	RaceName = GemRB.GetTableRowName(RaceTable, RaceColumn)
	# could use column ID as well, but they tend to change :)
	RaceColumn = GemRB.GetTableValue(RaceTable, RaceName, "SKILL_COLUMN")

	Class = GemRB.GetVar("Class") - 1
	ClassTable = GemRB.LoadTable("classes")
	KitName = GemRB.GetTableRowName(ClassTable, Class)
	#classcolumn is base class
	ClassColumn = GemRB.GetTableValue(ClassTable, Class, 3) - 1
	if ClassColumn < 0:  #it was already a base class
		ClassColumn = Class

	FeatTable = GemRB.LoadTable("feats")
	RowCount = GemRB.GetTableRowCount(FeatTable)

	for i in range(RowCount):
			GemRB.SetVar("Feat "+str(i),0)

	FeatLevelTable = GemRB.LoadTable("featlvl")
	FeatClassTable = GemRB.LoadTable("featclas")
	#calculating the number of new feats for the next level
	PointsLeft = 0
	#levels start with 1
	Level = GemRB.GetVar("Level")-1

	#this one exists only for clerics
	# Although it should be made extendable to all kits
	# A FEAT_COLUMN is needed in classes.2da or better yet, a whole new 2da
	ClassColumn += 3
	if KitColumn:
		KitColumn = 3 + KitColumn + 11

	#Always raise one level at once
	PointsLeft += GemRB.GetTableValue(FeatLevelTable, Level, 0)
	PointsLeft += GemRB.GetTableValue(FeatClassTable, Level, ClassColumn)
	
	#racial abilities which seem to be hardcoded in the IWD2 engine
	#are implemented in races.2da
	if Level<1:
		TmpTable = GemRB.LoadTable('races')
		PointsLeft += GemRB.GetTableValue(TmpTable,RaceName,'FEATBONUS')
		GemRB.UnloadTable(TmpTable)
	###
	
	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG", 800, 600)
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
		for j in range(0,5):
			Star=GemRB.GetControl(FeatWindow, i*5+j+36)
			GemRB.SetButtonState(FeatWindow, Star, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonFlags(FeatWindow, Star, IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	BackButton = GemRB.GetControl(FeatWindow,105)
	GemRB.SetText(FeatWindow,BackButton,15416)
	DoneButton = GemRB.GetControl(FeatWindow,0)
	GemRB.SetText(FeatWindow,DoneButton,36789)
	GemRB.SetButtonFlags(FeatWindow, DoneButton, IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = GemRB.GetControl(FeatWindow, 92)
	GemRB.SetText(FeatWindow,TextAreaControl,36476)

	ScrollBarControl = GemRB.GetControl(FeatWindow, 104)
	GemRB.SetEvent(FeatWindow, ScrollBarControl,IE_GUI_SCROLLBAR_ON_CHANGE,"ScrollBarPress")
	#decrease it with the number of controls on screen (list size)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	GemRB.SetVarAssoc(FeatWindow, ScrollBarControl, "TopIndex",RowCount-10)

	GemRB.SetEvent(FeatWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(FeatWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
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

	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
	if ActPoint <= 0:
		return
	GemRB.SetVar("Feat "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawFeats()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex+1

	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	if PointsLeft < 1:
		return
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
	if ActPoint > Level: #Level is 0 for level 1
		return
	GemRB.SetVar("Feat "+str(Pos), ActPoint+1)
	PointsLeft = PointsLeft - 1 
	RedrawFeats()
	return

def BackPress():
	GemRB.UnloadWindow(FeatWindow)
	for i in range(GemRB.GetTableRowCount(FeatTable)):
		GemRB.SetVar("Feat "+str(i),0)
	GemRB.SetNextScript("Skills")
	return

def NextPress():
	GemRB.UnloadWindow(FeatWindow)
	GemRB.SetNextScript("CharGen7")
	return

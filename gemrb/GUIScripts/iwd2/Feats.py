#character generation, skills (GUICG6)
import GemRB

FeatWindow = 0
TextAreaControl = 0
DoneButton = 0
FeatTable = 0
FeatReqTable = 0
TopIndex = 0
Level = 0
ClassColumn = 0
KitColumn = 0
RaceColumn = 0
FeatsClassColumn = 0
PointsLeft = 0

# returns the number of feat levels (for example cleave can be taken twice)
def MultiLevelFeat(feat):
	global FeatReqTable
	return GemRB.GetTableValue(FeatReqTable, feat, "MAX_LEVEL")

# FIXME: CheckFeatCondition doesn't check for higher level prerequisites
# (eg. cleave2 needs +4 BAB and weapon specialisation needs 4 fighter levels)

# NOTE: cleave formula is now:
# HITBONUS>=4 OR FEAT_CLEAVE<1
#
# specialisation formulas:
# FIGHTERLEVEL>=4 OR FEAT_*<2
# The default operator was set to 4 (greater or equal), so the majority of the formulas
# don't need any more change
# Avenger

def IsFeatUsable(feat):
	global FeatReqTable

	a_value = GemRB.GetTableValue(FeatReqTable, feat, "A_VALUE")
	if a_value<0:
		#string
		a_stat = GemRB.GetTableValue(FeatReqTable, feat, "A_STAT", 0)
	else:
		#stat
		a_stat = GemRB.GetTableValue(FeatReqTable, feat, "A_STAT",2)
	b_stat = GemRB.GetTableValue(FeatReqTable, feat, "B_STAT",2)
	c_stat = GemRB.GetTableValue(FeatReqTable, feat, "C_STAT",2)
	d_stat = GemRB.GetTableValue(FeatReqTable, feat, "D_STAT",2)
	b_value = GemRB.GetTableValue(FeatReqTable, feat, "B_VALUE")
	c_value = GemRB.GetTableValue(FeatReqTable, feat, "C_VALUE")
	d_value = GemRB.GetTableValue(FeatReqTable, feat, "D_VALUE")
	a_op = GemRB.GetTableValue(FeatReqTable, feat, "A_OP")
	b_op = GemRB.GetTableValue(FeatReqTable, feat, "B_OP")
	c_op = GemRB.GetTableValue(FeatReqTable, feat, "C_OP")
	d_op = GemRB.GetTableValue(FeatReqTable, feat, "D_OP")
	slot = GemRB.GetVar("Slot")
	return GemRB.CheckFeatCondition(slot, a_stat, a_value, b_stat, b_value, c_stat, c_value, d_stat, d_value, a_op, b_op, c_op, d_op)

# checks if a feat was granted due to class/kit/race and returns the number
# of granted levels. The bonuses aren't cumulative.
def GetBaseValue(feat):
	global FeatsClassColumn, RaceColumn, KitName

	Val1 = GemRB.GetTableValue(FeatTable, feat, FeatsClassColumn)
	Val2 = GemRB.GetTableValue(FeatTable, feat, RaceColumn)
	if Val2 < Val1:
		Val = Val1
	else:
		Val = Val2

	Val3 = 0
	# only cleric kits have feat bonuses in the original, but the column names are shortened
	KitName = KitName.replace("CLERIC_","C_")
	KitColumn = GemRB.GetTableColumnIndex(FeatTable, KitName)
	if KitColumn != 0:
		Val3 = GemRB.GetTableValue(FeatTable, feat, KitColumn)
		if Val3 > Val:
			Val = Val3

	return Val

def RedrawFeats():
	global TopIndex, PointsLeft, FeatWindow, FeatReqTable

	SumLabel = GemRB.GetControl(FeatWindow, 0x1000000c)
	if PointsLeft == 0:
		GemRB.SetButtonState(FeatWindow, DoneButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetLabelTextColor(FeatWindow,SumLabel, 255, 255, 255)
	else:
		GemRB.SetButtonState(FeatWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
		GemRB.SetLabelTextColor(FeatWindow,SumLabel, 255, 255, 0)

	GemRB.SetText(FeatWindow, SumLabel, str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i
		FeatName = GemRB.GetTableValue(FeatTable, Pos, 1)
		Label = GemRB.GetControl(FeatWindow, 0x10000001+i)
		GemRB.SetText(FeatWindow, Label, FeatName)

		FeatName=GemRB.GetTableRowName(FeatTable, Pos) #row name
		FeatValue = GemRB.GetVar("Feat "+str(Pos))

		ButtonPlus = GemRB.GetControl(FeatWindow, i*2+14)
		ButtonMinus = GemRB.GetControl(FeatWindow, i*2+15)
		if FeatValue == 0:
			GemRB.SetButtonState(FeatWindow, ButtonMinus, IE_GUI_BUTTON_DISABLED)
			# check if feat is usable - can be taken
			if IsFeatUsable(FeatName):
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_ENABLED)
				GemRB.SetLabelTextColor(FeatWindow, Label, 255, 255, 255)
			else:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
				GemRB.SetLabelTextColor(FeatWindow, Label, 150, 150, 150)
		else:
			# check for maximum if there are more feat levels
			# FIXME also verify that the next level of the feat is usable
			if MultiLevelFeat(FeatName) > FeatValue:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_ENABLED)
				GemRB.SetLabelTextColor(FeatWindow, Label, 255, 255, 255)
			else:
				GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
				GemRB.SetLabelTextColor(FeatWindow, Label, 150, 150, 150)
			GemRB.SetButtonState(FeatWindow, ButtonMinus, IE_GUI_BUTTON_ENABLED)

		if PointsLeft == 0:
			GemRB.SetButtonState(FeatWindow, ButtonPlus, IE_GUI_BUTTON_DISABLED)
			GemRB.SetLabelTextColor(FeatWindow, Label, 150, 150, 150)

		levels = GemRB.GetTableValue(FeatReqTable, FeatName, "MAX_LEVEL")
		FeatValueCounter = FeatValue
		# count backwards, since the controls follow each other in rtl order,
		# while we need to change the bams in ltr order
		for j in range(4, -1, -1):
			Star = GemRB.GetControl(FeatWindow, i*5+j+36)
			if 5 - j - 1 < levels:
				# the star should be there, but which one?
				if FeatValueCounter > 0:
					# the full one - the character has already taken a level of this feat
					GemRB.SetButtonState(FeatWindow, Star, IE_GUI_BUTTON_LOCKED)
					GemRB.SetButtonBAM(FeatWindow, Star, "GUIPFC", 0, 0, -1)
					GemRB.SetButtonFlags(FeatWindow, Star, IE_GUI_BUTTON_PICTURE, OP_OR)
					FeatValueCounter = FeatValueCounter - 1
				else:
					# the empty one - the character hasn't taken any levels of this feat yet
					GemRB.SetButtonState(FeatWindow, Star, IE_GUI_BUTTON_LOCKED)
					GemRB.SetButtonBAM(FeatWindow, Star, "GUIPFC", 0, 1, -1)
					GemRB.SetButtonFlags(FeatWindow, Star, IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				# no star, no bad bam crap
				GemRB.SetButtonState(FeatWindow, Star, IE_GUI_BUTTON_DISABLED)
				GemRB.SetButtonFlags(FeatWindow, Star, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				GemRB.SetButtonFlags(FeatWindow, Star, IE_GUI_BUTTON_PICTURE, OP_NAND)
	return

def ScrollBarPress():
	global TopIndex

	TopIndex = GemRB.GetVar("TopIndex")
	RedrawFeats()
	return

def OnLoad():
	global FeatWindow, TextAreaControl, DoneButton, TopIndex
	global FeatTable, FeatReqTable
	global KitName, Level, PointsLeft
	global ClassColumn, KitColumn, RaceColumn, FeatsClassColumn
	
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
	# classcolumn is base class or 0 if it is not a kit
	ClassColumn = GemRB.GetTableValue(ClassTable, Class, 3) - 1
	if ClassColumn < 0:  #it was already a base class
		ClassColumn = Class
		FeatsClassColumn = GemRB.GetTableValue(ClassTable, Class, 2) + 2
	else:
		FeatsClassColumn = GemRB.GetTableValue(ClassTable, Class, 3) + 2

	FeatTable = GemRB.LoadTable("feats")
	RowCount = GemRB.GetTableRowCount(FeatTable)
	FeatReqTable = GemRB.LoadTable("featreq")

	for i in range(RowCount):
		GemRB.SetVar("Feat "+str(i), GetBaseValue(i))

	FeatLevelTable = GemRB.LoadTable("featlvl")
	FeatClassTable = GemRB.LoadTable("featclas")
	#calculating the number of new feats for the next level
	PointsLeft = 0
	#levels start with 1
	Level = GemRB.GetVar("Level")-1

	#this one exists only for clerics
	# Although it should be made extendable to all kits
	# A FEAT_COLUMN is needed in classes.2da or better yet, a whole new 2da
	if GemRB.GetTableValue(ClassTable, Class, 3) == "CLERIC":
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
	for i in range(10):
		Button = GemRB.GetControl(FeatWindow, i+93)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "JustPress")

		Button = GemRB.GetControl(FeatWindow, i*2+14)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "LeftPress")

		Button = GemRB.GetControl(FeatWindow, i*2+15)
		GemRB.SetVarAssoc(FeatWindow, Button, "Feat",i)
		GemRB.SetEvent(FeatWindow, Button, IE_GUI_BUTTON_ON_PRESS, "RightPress")
		for j in range(5):
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
	GemRB.SetDefaultScrollBar (FeatWindow, ScrollBarControl)

	GemRB.SetEvent(FeatWindow,DoneButton,IE_GUI_BUTTON_ON_PRESS,"NextPress")
	GemRB.SetEvent(FeatWindow,BackButton,IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RedrawFeats()
	GemRB.SetVisible(FeatWindow,1)
	return


def JustPress():
	Pos = GemRB.GetVar("Feat")+TopIndex
	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex

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

	Pos = GemRB.GetVar("Feat")+TopIndex

	GemRB.SetText(FeatWindow, TextAreaControl, GemRB.GetTableValue(FeatTable,Pos,2) )
	if PointsLeft < 1:
		return
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
#	if ActPoint > Level: #Level is 0 for level 1
#		return
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

#Custom feat check functions
def Check_AnyOfThree(pl, a, as, b, bs, c, cs, *garbage):
	if GemRB.GetPlayerStat(pl, as)==a: return True
	if GemRB.GetPlayerStat(pl, bs)==b: return True
	if GemRB.GetPlayerStat(pl, cs)==c: return True
	return False

# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation, skills (GUICG6)
import GemRB
import GUICommon
import CommonTables
from GUIDefines import *
from ie_stats import *

FeatWindow = 0
TextAreaControl = 0
DoneButton = 0
FeatTable = 0
FeatReqTable = 0
TopIndex = 0
Level = 0
KitColumn = 0
RaceColumn = 0
FeatsClassColumn = 0
PointsLeft = 0

# returns the number of feat levels (for example cleave can be taken twice)
def MultiLevelFeat(feat):
	global FeatReqTable
	return FeatReqTable.GetValue(feat, "MAX_LEVEL")

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

	a_value = FeatReqTable.GetValue(feat, "A_VALUE")
	if a_value<0:
		#string
		a_stat = FeatReqTable.GetValue(feat, "A_STAT", GTV_STR)
	else:
		#stat
		a_stat = FeatReqTable.GetValue(feat, "A_STAT", GTV_STAT)
	b_stat = FeatReqTable.GetValue(feat, "B_STAT", GTV_STAT)
	c_stat = FeatReqTable.GetValue(feat, "C_STAT", GTV_STAT)
	d_stat = FeatReqTable.GetValue(feat, "D_STAT", GTV_STAT)
	b_value = FeatReqTable.GetValue(feat, "B_VALUE")
	c_value = FeatReqTable.GetValue(feat, "C_VALUE")
	d_value = FeatReqTable.GetValue(feat, "D_VALUE")
	a_op = FeatReqTable.GetValue(feat, "A_OP")
	b_op = FeatReqTable.GetValue(feat, "B_OP")
	c_op = FeatReqTable.GetValue(feat, "C_OP")
	d_op = FeatReqTable.GetValue(feat, "D_OP")
	slot = GemRB.GetVar("Slot")

	return GemRB.CheckFeatCondition(slot, a_stat, a_value, b_stat, b_value, c_stat, c_value, d_stat, d_value, a_op, b_op, c_op, d_op)

# checks if a feat was granted due to class/kit/race and returns the number
# of granted levels. The bonuses aren't cumulative.
def GetBaseValue(feat):
	global FeatsClassColumn, RaceColumn, KitName

	Val1 = FeatTable.GetValue(feat, FeatsClassColumn)
	Val2 = FeatTable.GetValue(feat, RaceColumn)
	if Val2 < Val1:
		Val = Val1
	else:
		Val = Val2

	Val3 = 0
	# only cleric kits have feat bonuses in the original, but the column names are shortened
	KitName = KitName.replace("CLERIC_","C_")
	KitColumn = FeatTable.GetColumnIndex(KitName)
	if KitColumn != 0:
		Val3 = FeatTable.GetValue(feat, KitColumn)
		if Val3 > Val:
			Val = Val3

	return Val

def RedrawFeats():
	global TopIndex, PointsLeft, FeatWindow, FeatReqTable

	SumLabel = FeatWindow.GetControl(0x1000000c)
	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
		SumLabel.SetTextColor(255, 255, 255)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
		SumLabel.SetTextColor(255, 255, 0)

	SumLabel.SetText(str(PointsLeft) )

	for i in range(0,10):
		Pos=TopIndex+i
		FeatName = FeatTable.GetValue(Pos, 1)
		Label = FeatWindow.GetControl(0x10000001+i)
		Label.SetText(FeatName)

		FeatName=FeatTable.GetRowName(Pos) #row name
		FeatValue = GemRB.GetVar("Feat "+str(Pos))

		ButtonPlus = FeatWindow.GetControl(i*2+14)
		ButtonMinus = FeatWindow.GetControl(i*2+15)
		if FeatValue == 0:
			ButtonMinus.SetState(IE_GUI_BUTTON_DISABLED)
			# check if feat is usable - can be taken
			if IsFeatUsable(FeatName):
				ButtonPlus.SetState(IE_GUI_BUTTON_ENABLED)
				Label.SetTextColor(255, 255, 255)
			else:
				ButtonPlus.SetState(IE_GUI_BUTTON_DISABLED)
				Label.SetTextColor(150, 150, 150)
		else:
			# check for maximum if there are more feat levels
			# FIXME also verify that the next level of the feat is usable
			if MultiLevelFeat(FeatName) > FeatValue:
				ButtonPlus.SetState(IE_GUI_BUTTON_ENABLED)
				Label.SetTextColor(255, 255, 255)
			else:
				ButtonPlus.SetState(IE_GUI_BUTTON_DISABLED)
				Label.SetTextColor(150, 150, 150)
			BaseValue = GemRB.GetVar("BaseFeatValue " + str(Pos))
			if FeatValue > BaseValue:
				ButtonMinus.SetState(IE_GUI_BUTTON_ENABLED)
			else:
				ButtonMinus.SetState(IE_GUI_BUTTON_DISABLED)

		if PointsLeft == 0:
			ButtonPlus.SetState(IE_GUI_BUTTON_DISABLED)
			Label.SetTextColor(150, 150, 150)

		levels = FeatReqTable.GetValue(FeatName, "MAX_LEVEL")
		FeatValueCounter = FeatValue
		# count backwards, since the controls follow each other in rtl order,
		# while we need to change the bams in ltr order
		for j in range(4, -1, -1):
			Star = FeatWindow.GetControl(i*5+j+36)
			if 5 - j - 1 < levels:
				# the star should be there, but which one?
				if FeatValueCounter > 0:
					# the full one - the character has already taken a level of this feat
					Star.SetState(IE_GUI_BUTTON_LOCKED)
					Star.SetBAM("GUIPFC", 0, 0, -1)
					Star.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)
					FeatValueCounter = FeatValueCounter - 1
				else:
					# the empty one - the character hasn't taken any levels of this feat yet
					Star.SetState(IE_GUI_BUTTON_LOCKED)
					Star.SetBAM("GUIPFC", 0, 1, -1)
					Star.SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				# no star, no bad bam crap
				Star.SetState(IE_GUI_BUTTON_DISABLED)
				Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				Star.SetFlags(IE_GUI_BUTTON_PICTURE, OP_NAND)
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
	global KitColumn, RaceColumn, FeatsClassColumn

	GemRB.SetVar("Level",1) #for simplicity

	Race = GemRB.GetVar("Race")
	RaceColumn = CommonTables.Races.FindValue(3, Race)
	RaceName = CommonTables.Races.GetRowName(RaceColumn)
	# could use column ID as well, but they tend to change :)
	RaceColumn = CommonTables.Races.GetValue(RaceName, "SKILL_COLUMN")

	ClassIndex = GemRB.GetVar("Class") - 1
	ClassName = KitName = GUICommon.GetClassRowName (ClassIndex, "index")
	# classcolumn is base class or 0 if it is not a kit
	ClassColumn = CommonTables.Classes.GetValue(ClassName, "CLASS") - 1
	if ClassColumn < 0:  #it was already a base class
		ClassColumn = ClassIndex
		# feats.2da is transposed, but has the same ordering
		FeatsClassColumn = ClassIndex + 3 # CommonTables.Classes.GetValue (ClassName, "ID") + 2
	else:
		FeatsClassColumn = ClassColumn + 3

	FeatTable = GemRB.LoadTable("feats")
	RowCount = FeatTable.GetRowCount()
	FeatReqTable = GemRB.LoadTable("featreq")

	for i in range(RowCount):
		GemRB.SetVar("Feat "+str(i), GetBaseValue(i))
		GemRB.SetVar("BaseFeatValue " + str(i), GetBaseValue(i))

	FeatLevelTable = GemRB.LoadTable("featlvl")
	FeatClassTable = GemRB.LoadTable("featclas")
	#calculating the number of new feats for the next level
	PointsLeft = 0
	#levels start with 1
	Level = GemRB.GetVar("Level")-1

	#this one exists only for clerics
	# Although it should be made extendable to all kits
	# A FEAT_COLUMN is needed in classes.2da or better yet, a whole new 2da
	if CommonTables.Classes.GetValue (ClassName, "CLASS") == CommonTables.Classes.GetValue ("CLERIC", "ID"):
		if KitColumn:
			# 3 to get to the class columns (feats.2da) and 11 to get to these cleric kit columns
			KitColumn = 3 + KitColumn + 11

	#Always raise one level at once
	PointsLeft += FeatLevelTable.GetValue(Level, 0)
	PointsLeft += FeatClassTable.GetValue(Level, ClassColumn)

	#racial abilities which seem to be hardcoded in the IWD2 engine
	#are implemented in races.2da
	if Level<1:
		PointsLeft += CommonTables.Races.GetValue(RaceName,'FEATBONUS')
	###

	GemRB.SetToken("number",str(PointsLeft) )

	GemRB.LoadWindowPack("GUICG", 800, 600)
	FeatWindow = GemRB.LoadWindow(55)
	for i in range(10):
		Button = FeatWindow.GetControl(i+93)
		Button.SetVarAssoc("Feat",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, JustPress)

		Button = FeatWindow.GetControl(i*2+14)
		Button.SetVarAssoc("Feat",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, LeftPress)

		Button = FeatWindow.GetControl(i*2+15)
		Button.SetVarAssoc("Feat",i)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, RightPress)
		for j in range(5):
			Star=FeatWindow.GetControl(i*5+j+36)
			Star.SetState(IE_GUI_BUTTON_DISABLED)
			Star.SetFlags(IE_GUI_BUTTON_NO_IMAGE,OP_OR)

	BackButton = FeatWindow.GetControl(105)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = FeatWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = FeatWindow.GetControl(92)
	TextAreaControl.SetText(36476)

	ScrollBarControl = FeatWindow.GetControl(104)
	ScrollBarControl.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, ScrollBarPress)
	#decrease it with the number of controls on screen (list size)
	TopIndex = 0
	GemRB.SetVar("TopIndex",0)
	ScrollBarControl.SetVarAssoc("TopIndex",RowCount-10)
	ScrollBarControl.SetDefaultScrollBar ()

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	RedrawFeats()
	FeatWindow.SetVisible(WINDOW_VISIBLE)
	return


def JustPress():
	Pos = GemRB.GetVar("Feat")+TopIndex
	TextAreaControl.SetText(FeatTable.GetValue(Pos,2) )
	return

def RightPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex

	TextAreaControl.SetText(FeatTable.GetValue(Pos,2) )
	ActPoint = GemRB.GetVar("Feat "+str(Pos) )
	BaseValue = GemRB.GetVar("BaseFeatValue " + str(Pos))
	if ActPoint <= 0 or ActPoint <= BaseValue:
		return
	GemRB.SetVar("Feat "+str(Pos),ActPoint-1)
	PointsLeft = PointsLeft + 1
	RedrawFeats()
	return

def LeftPress():
	global PointsLeft

	Pos = GemRB.GetVar("Feat")+TopIndex

	TextAreaControl.SetText(FeatTable.GetValue(Pos,2) )
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
	if FeatWindow:
		FeatWindow.Unload()
	for i in range(FeatTable.GetRowCount()):
		GemRB.SetVar("Feat "+str(i),0)
	GemRB.SetNextScript("Skills")
	return

def NextPress():
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	if FeatWindow:
		FeatWindow.Unload()
	GemRB.SetNextScript("Spells")
	return

#Custom feat check functions
def Check_AnyOfThree(pl, ass, a, bs, b, cs, c, *garbage):

	if GemRB.GetPlayerStat(pl, ass)==a: return True
	if GemRB.GetPlayerStat(pl, bs)==b: return True
	if GemRB.GetPlayerStat(pl, cs)==c: return True
	return False

#Custom feat check functions
def Check_AnyOfThreeGE(pl, ass, a, bs, b, cs, c, *garbage):

	if GemRB.GetPlayerStat(pl, ass)>=a: return True
	if GemRB.GetPlayerStat(pl, bs)>=b: return True
	if GemRB.GetPlayerStat(pl, cs)>=c: return True
	return False

def Check_AllOfThreeGE(pl, ass, a, bs, b, cs, c, *garbage):

	if GemRB.GetPlayerStat(pl, ass) < a: return False
	if GemRB.GetPlayerStat(pl, bs) < b: return False
	if GemRB.GetPlayerStat(pl, cs) < c: return False
	return True

def Check_IsCaster(pl, *garbage):
	# CLASSLEVELMAGE is IE_LEVEL2 (pst)
	possible_casters = { IE_LEVEL2:1, IE_LEVELCLERIC:1, IE_LEVELDRUID:1,
		IE_LEVELSORCERER:1, IE_LEVELPALADIN:4, IE_LEVELRANGER:4, IE_LEVELBARD:2 }
	Caster = False

	for stat in possible_casters:
		if GemRB.GetPlayerStat(pl, stat) >= possible_casters[stat]:
			Caster = True
			break

	return Caster

# besides Concentration > 3, this feat requires Weapon Specialization in 2 weapons.
def Check_MaximizedAttacks(pl, a, ass, *garbage):
	if GemRB.GetPlayerStat(pl, ass) < a: return False
	# tuple of all weapon proficiency types
	proficiencies = ( IE_FEAT_BASTARDSWORD, IE_FEAT_AXE, IE_FEAT_BOW, IE_FEAT_FLAIL,
		IE_FEAT_GREAT_SWORD, IE_FEAT_HAMMER, IE_FEAT_LARGE_SWORD, IE_FEAT_POLEARM )
	SpecializationCount = 0

	for proficiency in proficiencies:
		if GemRB.GetPlayerStat(pl, proficiency) == 3:
			SpecializationCount += 1

	return SpecializationCount >= 2

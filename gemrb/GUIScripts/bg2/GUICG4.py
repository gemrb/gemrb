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
# $Id$
#
#character generation, ability (GUICG4)
import GemRB
from GUICommon import *

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
Abclasrq = 0
Abclsmod = 0
Abclasrq = 0
Abracerq = 0
PointsLeft = 0
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0
HasStrExtra = 0
MyChar = 0

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	Race = RaceTable.FindValue (1, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = RaceTable.GetRowName(Race)

	Minimum = 3
	Maximum = 18

	tmp = Abclasrq.GetValue(KitIndex, Abidx)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	Race = Abracerq.GetRowIndex(RaceName)
	tmp = Abracerq.GetValue(Race, Abidx*2)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	tmp = Abracerq.GetValue(Race, Abidx*2+1)
	if tmp!=0 and tmp>Maximum:
		Maximum = tmp

	Race = Abracead.GetRowIndex(RaceName)
	Add = Abracead.GetValue(Race, Abidx) + Abclsmod.GetValue(KitIndex, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1
	if Maximum>25:
		Maximum=25

	return

def RollPress():
	global Minimum, Maximum, Add, HasStrExtra, PointsLeft

	AbilityWindow.Invalidate()
	GemRB.SetVar("Ability",0)
	GemRB.SetVar("Ability -1",0)
	PointsLeft = 0
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText("0")
	SumLabel.SetUseRGB(1)

	if HasStrExtra:
		e = GemRB.Roll(1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)
	for i in range(6):
		dice = 3
		size = 5
		CalcLimits(i)
		v = GemRB.Roll(dice, size, Add+3)
		if v<Minimum:
			v = Minimum
		if v>Maximum:
			v = Maximum
		GemRB.SetVar("Ability "+str(i), v )
		Label = AbilityWindow.GetControl(0x10000003+i)
		if i==0 and v==18 and HasStrExtra:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(v) )
		Label.SetUseRGB(1)
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft, HasStrExtra
	global AbilityTable, Abclasrq, Abclsmod, Abracerq, Abracead
	global KitIndex, Minimum, Maximum, MyChar
	
	Abracead = GemRB.LoadTableObject("ABRACEAD")
	Abclsmod = GemRB.LoadTableObject("ABCLSMOD")
	Abclasrq = GemRB.LoadTableObject("ABCLASRQ")
	Abracerq = GemRB.LoadTableObject("ABRACERQ")

	MyChar = GemRB.GetVar ("Slot")
	Kit = GetKitIndex (MyChar)
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	if Kit == 0:
		KitName = ClassTable.GetRowName(Class)
	else:
		#rowname is just a number, first value row what we need here
		KitName = KitListTable.GetValue(Kit, 0)

	if ClassTable.GetValue(Class, 3)=="SAVEWAR":
		HasStrExtra=1
	else:
		HasStrExtra=0

	KitIndex = Abclasrq.GetRowIndex(KitName)

	GemRB.LoadWindowPack("GUICG", 640, 480)
	AbilityTable = GemRB.LoadTableObject("ability")
	AbilityWindow = GemRB.LoadWindowObject(4)

	RerollButton = AbilityWindow.GetControl(2)
	RerollButton.SetText(11982)
	StoreButton = AbilityWindow.GetControl(37)
	StoreButton.SetText(17373)
	RecallButton = AbilityWindow.GetControl(38)
	RecallButton.SetText(17374)

	BackButton = AbilityWindow.GetControl(36)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = AbilityWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	RollPress()
	StorePress()
	for i in range(6):
		Label = AbilityWindow.GetControl(i+0x10000009)
		Label.SetEvent(IE_GUI_LABEL_ON_PRESS, "OverPress"+str(i) )
		Button = AbilityWindow.GetControl(i+30)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "JustPress")
		Button.SetEvent(IE_GUI_MOUSE_LEAVE_BUTTON, "EmptyPress")
		Button.SetVarAssoc("Ability", i)

		Button = AbilityWindow.GetControl(i*2+16)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "LeftPress")
		Button.SetVarAssoc("Ability", i )

		Button = AbilityWindow.GetControl(i*2+17)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "RightPress")
		Button.SetVarAssoc("Ability", i )

	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)

	StoreButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"StorePress")
	RecallButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RecallPress")
	RerollButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RollPress")
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	AbilityWindow.SetVisible(1)
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_NAND)
	return

def RightPress():
	global PointsLeft

	AbilityWindow.Invalidate()
	Abidx = GemRB.GetVar("Ability")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	#should be more elaborate
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	if Ability<=Minimum:
		return
	GemRB.SetVar("Ability "+str(Abidx), Ability-1)
	PointsLeft = PointsLeft + 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText(str(PointsLeft) )
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	StrExtra = GemRB.GetVar("StrExtra")
	if Abidx==0 and Ability==19 and StrExtra:
		Label.SetText("18/"+str(StrExtra) )
	else:
		Label.SetText(str(Ability-1) )
	return

def JustPress():
	Abidx = GemRB.GetVar("Ability")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	#should be more elaborate
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	return

def LeftPress():
	global PointsLeft, HasStrExtra

	Abidx = GemRB.GetVar("Ability")
	AbilityWindow.Invalidate()
	PointsLeft=GemRB.GetVar("Ability -1")
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	if PointsLeft == 0:
		return
	if Ability>=Maximum:  #should be more elaborate
		return
	GemRB.SetVar("Ability "+str(Abidx), Ability+1)
	PointsLeft = PointsLeft - 1
	GemRB.SetVar("Ability -1",PointsLeft)
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText(str(PointsLeft) )
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	StrExtra = GemRB.GetVar("StrExtra")
	if Abidx==0 and Ability==17 and HasStrExtra==1:
		Label.SetText("18/%02d"%(StrExtra) )
	else:
		Label.SetText(str(Ability+1) )
	return

def EmptyPress():
	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)
	return

def StorePress():
	GemRB.SetVar("StoredStrExtra",GemRB.GetVar("StrExtra") )
	for i in range(-1,6):
		GemRB.SetVar("Stored "+str(i),GemRB.GetVar("Ability "+str(i) ) )
	return

def RecallPress():
	global PointsLeft

	AbilityWindow.Invalidate()
	e=GemRB.GetVar("StoredStrExtra")
	GemRB.SetVar("StrExtra",e)
	for i in range(-1,6):
		v = GemRB.GetVar("Stored "+str(i) )
		GemRB.SetVar("Ability "+str(i), v)
		Label = AbilityWindow.GetControl(0x10000003+i)
		if i==0 and v==18 and HasStrExtra==1:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(v) )

	PointsLeft = GemRB.GetVar("Ability -1")
	return

def BackPress():
	if AbilityWindow:
		AbilityWindow.Unload()
	GemRB.SetNextScript("CharGen5")
	GemRB.SetVar("StrExtra",0)
	for i in range(-1,6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	return

def NextPress():
	if AbilityWindow:
		AbilityWindow.Unload()
	# save our previous stats:
	#       abilities
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()

	# print our diagnostic as we loop (so as not to duplicate)
	print "CharGen6 output:"
	
	MyChar = GemRB.GetVar ("Slot")

	for i in range (AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		StatName = AbilityTable.GetRowName (i)
		StatValue = GemRB.GetVar ("Ability "+str(i))
		GemRB.SetPlayerStat (MyChar, StatID, StatValue)
		print "\t",StatName,":\t", StatValue

	# TODO: don't all chars have an str mod, even if it isn't applied?
	#       so it should be the cores duty to decide whether or not the char
	#       has 18 str in game and adjust accordingly; you wouldn't want an
	#       18/00 char use draw upon holy might to boost his str, then have
	#       it re-roll when it comes back to normal
	# apply our extra str
	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))
	print "\tSTREXTRA:\t",GemRB.GetVar ("StrExtra")

	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	GemRB.SetNextScript("CharGen6") #
	return

def OverPress0():
	Ability = GemRB.GetVar("Ability 0")
	CalcLimits(0)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(0, 1) )
	return

def OverPress1():
	Ability = GemRB.GetVar("Ability 1")
	CalcLimits(1)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(1, 1) )
	return

def OverPress2():
	Ability = GemRB.GetVar("Ability 2")
	CalcLimits(2)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(2, 1) )
	return

def OverPress3():
	Ability = GemRB.GetVar("Ability 3")
	CalcLimits(3)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(3, 1) )
	return

def OverPress4():
	Ability = GemRB.GetVar("Ability 4")
	CalcLimits(4)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(4, 1) )
	return

def OverPress5():
	Ability = GemRB.GetVar("Ability 5")
	CalcLimits(5)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(5, 1) )
	return

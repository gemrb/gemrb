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

from CharGenCommon import * 
from GUICommon import CloseOtherWindow
from GUICommonWindows import SetupHP


AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 0
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	RaceTable = GemRB.LoadTableObject("races")
	Abracead = GemRB.LoadTableObject("ABRACEAD")
	#Abclsmod = GemRB.LoadTableObject("ABCLSMOD")
	Race = GemRB.GetVar("Race")-1
	RaceName = RaceTable.GetRowName(Race)

	Minimum = 3
	Maximum = 18

	Abclasrq = GemRB.LoadTableObject("ABCLASRQ")
	tmp = Abclasrq.GetValue(KitIndex, Abidx)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	Abracerq = GemRB.LoadTableObject("ABRACERQ")
	Race = Abracerq.GetRowIndex(RaceName)
	tmp = Abracerq.GetValue(Race, Abidx*2)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	tmp = Abracerq.GetValue(Race, Abidx*2+1)
	if tmp!=0 and tmp>Maximum:
		Maximum = tmp

	Race = Abracead.GetRowIndex(RaceName)
	Add = Abracead.GetValue(Race, Abidx)# + Abclsmod.GetValue(KitIndex, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1
	if Maximum>25:
		Maximum=25

	return

def RollPress():
	global Minimum, Maximum, Add, PointsLeft

	AbilityWindow.Invalidate()
	GemRB.SetVar("Ability",0)
	GemRB.SetVar("Ability -1",0)
	PointsLeft = 0
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText("0")
	SumLabel.SetUseRGB(1)

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
		Label.SetText(str(v) )
		Label.SetUseRGB(1)
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft
	global AbilityTable
	global KitIndex, Minimum, Maximum

	if CloseOtherWindow (OnLoad):
		if(AbilityWindow):
			AbilityWindow.Unload()
			AbilityWindow = None
		return

	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	
	Abclasrq = GemRB.LoadTableObject("ABCLASRQ")

	
	ClassTable = GemRB.LoadTableObject("classes")
	Class = GemRB.GetVar("Class")-1
	KitName = ClassTable.GetRowName(Class)
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
	DoneButton = AbilityWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	RollPress()
	StorePress()
	for i in range(6):
		Button = AbilityWindow.GetControl(i+30)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "JustPress")
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
	AbilityWindow.ShowModal(MODAL_SHADOW_NONE)
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
	Label.SetText(str(Ability-1) )
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
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
	global PointsLeft

	AbilityWindow.Invalidate()
	PointsLeft=GemRB.GetVar("Ability -1")
	#should be more elaborate
	Minimum=3
	Maximum=18
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	Abidx = GemRB.GetVar("Ability")
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
	Label.SetText(str(Ability+1) )
	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def EmptyPress():
	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)
	return

def StorePress():
	for i in range(-1,6):
		GemRB.SetVar("Stored "+str(i),GemRB.GetVar("Ability "+str(i) ) )
	return

def RecallPress():
	global PointsLeft

	AbilityWindow.Invalidate()
	for i in range(-1,6):
		v = GemRB.GetVar("Stored "+str(i) )
		GemRB.SetVar("Ability "+str(i), v)
		Label = AbilityWindow.GetControl(0x10000003+i)
		Label.SetText(str(v) )

	PointsLeft = GemRB.GetVar("Ability -1")
	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def BackPress():
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	back()

def NextPress():
	GemRB.SetRepeatClickFlags(GEM_RK_DISABLE, OP_OR)
	
	MyChar = GemRB.GetVar ("Slot")
	
	AbilityTable = GemRB.LoadTableObject ("ability")
	AbilityCount = AbilityTable.GetRowCount ()
	
	for i in range (AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		StatValue = GemRB.GetVar ("Ability "+str(i))
		GemRB.SetPlayerStat (MyChar, StatID, StatValue)
	SetupHP(MyChar)
	next()


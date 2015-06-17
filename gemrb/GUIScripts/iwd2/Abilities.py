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
#character generation, ability (GUICG4)
import GemRB
from GUIDefines import *
import CommonTables

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 16
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	Abracead = GemRB.LoadTable("ABRACEAD")
	RaceID = GemRB.GetVar("Race")
	RowIndex = CommonTables.Races.FindValue(3, RaceID)
	RaceName = CommonTables.Races.GetRowName(RowIndex)

	Minimum = 3
	Maximum = 18

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	tmp = Abclasrq.GetValue(KitIndex, Abidx)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	Abracerq = GemRB.LoadTable("ABRACERQ")
	Race = Abracerq.GetRowIndex(RaceName)
	tmp = Abracerq.GetValue(Race, Abidx*2)
	if tmp!=0 and tmp>Minimum:
		Minimum = tmp

	tmp = Abracerq.GetValue(Race, Abidx*2+1)
	if tmp!=0 and tmp>Maximum:
		Maximum = tmp

	Race = Abracead.GetRowIndex(RaceName)
	Add = Abracead.GetValue(Race, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1

	return

def RollPress():
	global PointsLeft, Add

	GemRB.SetVar("Ability",0)
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetTextColor(255, 255, 0)
	PointsLeft=16
	SumLabel.SetUseRGB(1)
	SumLabel.SetText(str(PointsLeft))

	for i in range(0,6):
		CalcLimits(i)
		v = 10+Add
		b = v//2-5
		GemRB.SetVar("Ability "+str(i), v )
		Label = AbilityWindow.GetControl(0x10000003+i)
		Label.SetText(str(v) )

		Label = AbilityWindow.GetControl(0x10000024+i)
		Label.SetUseRGB(1)
		if b<0:
			Label.SetTextColor(255,0,0)
		elif b>0:
			Label.SetTextColor(0,255,0)
		else:
			Label.SetTextColor(255,255,255)
		Label.SetText("%+d"%(b))
	return

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft
	global AbilityTable
	global KitIndex, Minimum, Maximum
	
	Kit = GemRB.GetVar("Class Kit")
	Class = GemRB.GetVar("Class")-1
	if Kit == 0:
		KitName = CommonTables.Classes.GetRowName(Class)
	else:
		#rowname is just a number, first value row what we need here
		KitName = CommonTables.KitList.GetValue(Kit, 0)

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	KitIndex = Abclasrq.GetRowIndex(KitName)

	GemRB.LoadWindowPack("GUICG", 800 ,600)
	AbilityTable = GemRB.LoadTable("ability")
	AbilityWindow = GemRB.LoadWindow(4)

	RollPress()
	for i in range(0,6):
		Button = AbilityWindow.GetControl(i+30)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, JustPress)
		Button.SetVarAssoc("Ability", i)

		Button = AbilityWindow.GetControl(i*2+16)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, LeftPress)
		Button.SetVarAssoc("Ability", i )

		Button = AbilityWindow.GetControl(i*2+17)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, RightPress)
		Button.SetVarAssoc("Ability", i )

	BackButton = AbilityWindow.GetControl(36)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = AbilityWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	AbilityWindow.SetVisible(WINDOW_VISIBLE)
	return

def RightPress():
	global PointsLeft

	Abidx = GemRB.GetVar("Ability")
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	#should be more elaborate
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	if Ability<=Minimum:
		return
	Ability -= 1
	GemRB.SetVar("Ability "+str(Abidx), Ability)
	PointsLeft = PointsLeft + 1
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText(str(PointsLeft) )
	SumLabel.SetTextColor(255, 255, 0)
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	Label.SetText(str(Ability) )
	Label = AbilityWindow.GetControl(0x10000024+Abidx)
	b = Ability // 2 - 5
	if b<0:
		Label.SetTextColor(255,0,0)
	elif b>0:
		Label.SetTextColor(0,255,0)
	else:
		Label.SetTextColor(255,255,255)
	Label.SetText("%+d"%(b))
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

	Abidx = GemRB.GetVar("Ability")
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	if PointsLeft == 0:
		return
	if Ability>=Maximum:  #should be more elaborate
		return
	Ability += 1
	GemRB.SetVar("Ability "+str(Abidx), Ability)
	PointsLeft = PointsLeft - 1
	SumLabel = AbilityWindow.GetControl(0x10000002)
	if PointsLeft == 0:
		SumLabel.SetTextColor(255, 255, 255)
	SumLabel.SetText(str(PointsLeft) )
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	Label.SetText(str(Ability) )
	Label = AbilityWindow.GetControl(0x10000024+Abidx)
	b = Ability // 2 - 5
	if b<0:
		Label.SetTextColor(255,0,0)
	elif b>0:
		Label.SetTextColor(0,255,0)
	else:
		Label.SetTextColor(255,255,255)
	Label.SetText("%+d"%(b))
	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if AbilityWindow:
		AbilityWindow.Unload()
	GemRB.SetNextScript("CharGen5")
	for i in range(6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
	if AbilityWindow:
		AbilityWindow.Unload()
	GemRB.SetNextScript("CharGen6") #skills
	return

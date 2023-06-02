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
import CharOverview
import CommonTables
from ie_stats import IE_STR, IE_DEX, IE_CON, IE_INT, IE_WIS, IE_CHR

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
AbilityTable = 0
PointsLeft = 0
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0
CharGen = 0
Stats = [ IE_STR, IE_DEX, IE_CON, IE_INT, IE_WIS, IE_CHR ]

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	if not CharGen:
		pc = GemRB.GetVar("SELECTED_PC")
		Minimum = GemRB.GetPlayerStat (pc, Stats[Abidx], 1)
		Maximum = 25
		return

	Abracead = GemRB.LoadTable("ABRACEAD")
	RaceID = GemRB.GetVar("Race")
	RowIndex = CommonTables.Races.FindValue(3, RaceID)
	RaceName = CommonTables.Races.GetRowName(RowIndex)

	Minimum = 3
	Maximum = 18

	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	tmp = Abclasrq.GetValue(KitIndex, Abidx)
	if tmp > Minimum:
		Minimum = tmp

	Abracerq = GemRB.LoadTable("ABRACERQ")
	Race = Abracerq.GetRowIndex(RaceName)
	tmp = Abracerq.GetValue(Race, Abidx*2)
	if tmp > Minimum:
		Minimum = tmp

	tmp = Abracerq.GetValue(Race, Abidx*2+1)
	if tmp > Maximum:
		Maximum = tmp

	Race = Abracead.GetRowIndex(RaceName)
	Add = Abracead.GetValue(Race, Abidx)
	Maximum = Maximum + Add
	Minimum = Minimum + Add
	if Minimum<1:
		Minimum=1

	return

def GetModColor(mod):
	if mod < 0:
		return {'r' : 255, 'g' : 0, 'b' : 0}
	elif mod > 0:
		return {'r' : 0, 'g' : 255, 'b' : 0}
	else:
		return {'r' : 255, 'g' : 255, 'b' : 255}

def RollPress():
	global Add

	GemRB.SetVar("Ability",0)
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})
	SumLabel.SetText(str(PointsLeft))

	for i in range(0,6):
		CalcLimits(i)
		v = 10+Add
		if not CharGen:
			v = Minimum
		b = v//2-5
		GemRB.SetVar("Ability "+str(i), v )
		Label = AbilityWindow.GetControl(0x10000003+i)
		Label.SetText(str(v) )

		Label = AbilityWindow.GetControl(0x10000024+i)
		Label.SetColor (GetModColor (b))
		Label.SetText("%+d"%(b))
	return

def OnLoad():
	OpenAbilitiesWindow (1, GemRB.GetVar ("DefaultCharGenPointsPool"))

def OpenAbilitiesWindow(chargen, points):
	global AbilityWindow, TextAreaControl, DoneButton
	global CharGen, PointsLeft
	global AbilityTable
	global KitIndex, Minimum, Maximum
	
	CharGen = chargen
	PointsLeft = points

	AbilityTable = GemRB.LoadTable ("ability")
	if chargen:
		Kit = GemRB.GetVar("Class Kit")
		Class = GemRB.GetVar("Class")-1
		if Kit == 0:
			KitName = CommonTables.Classes.GetRowName(Class)
		else:
			#rowname is just a number, first value row what we need here
			KitName = CommonTables.KitList.GetValue(Kit, 0)

		Abclasrq = GemRB.LoadTable("ABCLASRQ")
		KitIndex = Abclasrq.GetRowIndex(KitName)
		# iwd2 comes with an effectively empty table, but let's allow for modding
		if KitIndex == None:
			KitIndex = 99999999

	# in a fit of clarity, they used the same ids in both windowpacks
	if chargen:
		AbilityWindow = GemRB.LoadWindow (4, "GUICG")
		CharOverview.PositionCharGenWin (AbilityWindow)
	else:
		AbilityWindow = GemRB.LoadWindow (7, "GUIREC")

	RollPress ()
	for i in range(0,6):
		Button = AbilityWindow.GetControl(i+30)
		Button.OnPress (JustPress)
		Button.SetValue (i)

		Button = AbilityWindow.GetControl(i*2+16)
		Button.OnPress (LeftPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

		Button = AbilityWindow.GetControl(i*2+17)
		Button.OnPress (RightPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

	if chargen:
		BackButton = AbilityWindow.GetControl (36)
		BackButton.SetText (15416)
		BackButton.MakeEscape()
		BackButton.OnPress (BackPress)
	else:
		AbilityWindow.DeleteControl (36)

	DoneButton = AbilityWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.OnPress (NextPress)

	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)

	if not chargen:
		AbilityWindow.ShowModal (MODAL_SHADOW_GRAY)
	else:
		AbilityWindow.Focus()
	return

def RightPress(btn):
	global PointsLeft

	Abidx = btn.Value
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
	SumLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	Label.SetText(str(Ability) )
	Label = AbilityWindow.GetControl(0x10000024+Abidx)
	b = Ability // 2 - 5
	Label.SetColor (GetModColor (b))
	Label.SetText("%+d"%(b))
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def JustPress(btn):
	Abidx = btn.Value
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
	TextAreaControl.SetText(AbilityTable.GetValue(Abidx, 1) )
	return

def LeftPress(btn):
	global PointsLeft

	Abidx = btn.Value
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
		SumLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
	SumLabel.SetText(str(PointsLeft) )
	Label = AbilityWindow.GetControl(0x10000003+Abidx)
	Label.SetText(str(Ability) )
	Label = AbilityWindow.GetControl(0x10000024+Abidx)
	b = Ability // 2 - 5
	Label.SetColor (GetModColor (b))
	Label.SetText("%+d"%(b))
	if PointsLeft == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if AbilityWindow:
		AbilityWindow.Close ()
	GemRB.SetNextScript("CharGen5")
	for i in range(6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
	if AbilityWindow:
		AbilityWindow.Close ()
	if CharGen:
		GemRB.SetNextScript("CharGen6") #skills
	else:
		# set the upgraded stats
		pc = GemRB.GetVar("SELECTED_PC")
		for i in range (len(Stats)):
			newValue = GemRB.GetVar ("Ability "+str(i))
			GemRB.SetPlayerStat (pc, Stats[i], newValue)
		# open up the next levelup window
		import Enemy
		Enemy.OpenEnemyWindow ()

	return

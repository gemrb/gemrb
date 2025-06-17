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
import GUICommon
import CommonTables
from ie_stats import *
from GUIDefines import *
from ie_restype import RES_2DA

import CharGenCommon

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

	Race = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE) )
	RaceName = CommonTables.Races.GetRowName(Race)

	Minimum = 3
	Maximum = 18

	Race = Abracerq.GetRowIndex(RaceName)
	tmp = Abracerq.GetValue(Race, Abidx*2)
	if tmp != 0:
		Minimum = tmp

	tmp = Abracerq.GetValue(Race, Abidx*2+1)
	if tmp != 0:
		Maximum = tmp

	Race = Abracead.GetRowIndex(RaceName)
	Add = Abracead.GetValue(Race, Abidx)
	Minimum += Add
	Maximum += Add

	if Abclsmod:
		tmp = Abclsmod.GetValue(KitIndex, Abidx)
		Maximum += tmp
		Add += tmp

	tmp = Abclasrq.GetValue(KitIndex, Abidx)
	if tmp > Minimum:
		Minimum = tmp

	if Minimum < 1:
		Minimum = 1
	if Maximum > 25:
		Maximum = 25

	return

def RollPress():
	global Minimum, Maximum, Add, HasStrExtra, PointsLeft
	global AllPoints18

	GemRB.SetVar("Ability",0)
	GemRB.SetVar("Ability -1",0)
	PointsLeft = 0
	SumLabel = AbilityWindow.GetControl(0x10000002)
	SumLabel.SetText("0")

	if HasStrExtra:
		if AllPoints18:
			e = 100
		else:
			e = GemRB.Roll(1,100,0)
	else:
		e = 0
	GemRB.SetVar("StrExtra", e)

	Total = 0
	while (Total < 75):
		Total = 0
		for i in range(6):
			dice = 3
			size = 5
			CalcLimits(i)
			v = GemRB.Roll(dice, size, Add+3)
			if v<Minimum:
				v = Minimum
			if v>Maximum:
				v = Maximum
			if AllPoints18:
				v = 18
			GemRB.SetVar("Ability "+str(i), v )
			Total += v

			Label = AbilityWindow.GetControl(0x10000003+i)
			if i==0 and v==18 and HasStrExtra:
				Label.SetText("18/"+str(e) )
			else:
				Label.SetText(str(v) )

	# add a counter to the title
	Sum = str(Total)
	SumLabel = AbilityWindow.GetControl (0x10000000)
	SumLabel.SetText (GemRB.GetString(11976) + ": " + Sum)
	SumLabel2 = AbilityWindow.GetControl (0x10000000 + 40)
	if SumLabel2:
		SumLabel2.SetText (Sum)
	AllPoints18 = 0
	return

def GiveAll18():
	global AllPoints18

	AllPoints18 = 1
	RollPress()

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global PointsLeft, HasStrExtra
	global AbilityTable, Abclasrq, Abclsmod, Abracerq, Abracead
	global KitIndex, Minimum, Maximum, MyChar
	global AllPoints18

	AllPoints18 = 0
	
	Abracead = GemRB.LoadTable("ABRACEAD")
	if GemRB.HasResource ("ABCLSMOD", RES_2DA):
		Abclsmod = GemRB.LoadTable ("ABCLSMOD")
	Abclasrq = GemRB.LoadTable("ABCLASRQ")
	Abracerq = GemRB.LoadTable("ABRACERQ")

	MyChar = GemRB.GetVar ("Slot")
	Kit = GUICommon.GetKitIndex (MyChar)
	ClassName = GUICommon.GetClassRowName (MyChar)
	if Kit == 0:
		KitName = ClassName
	else:
		#rowname is just a number, first value row what we need here
		KitName = CommonTables.KitList.GetValue(Kit, 0)

	#if the class uses the warrior table for saves, then it may have the extra strength
	HasStrExtra = CommonTables.Classes.GetValue(ClassName, "STREXTRA", GTV_INT)

	KitIndex = Abclasrq.GetRowIndex(KitName)
	if KitIndex == None:
		KitIndex = 9999999 # for broken mods / installs; both tables have 0 defaults which work fine

	AbilityTable = GemRB.LoadTable("ability")
	AbilityWindow = GemRB.LoadWindow(4, "GUICG")
	CharGenCommon.PositionCharGenWin(AbilityWindow)

	Button = AbilityWindow.CreateButton (2000, 0, 0, 0, 0)
	Button.OnPress (GiveAll18)
	Button.SetHotKey ("8", GEM_MOD_CTRL, True)

	RerollButton = AbilityWindow.GetControl(2)
	RerollButton.SetText(11982)
	StoreButton = AbilityWindow.GetControl(37)
	StoreButton.SetText(17373)
	RecallButton = AbilityWindow.GetControl(38)
	RecallButton.SetText(17374)

	BackButton = AbilityWindow.GetControl(36)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = AbilityWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	RollPress()
	StorePress()
	for i in range(6):
		Button = AbilityWindow.GetControl(i+30)
		Button.OnPress (JustPress)
		Button.OnMouseLeave (EmptyPress)
		Button.SetValue (i)
		# delete the labels and use the buttons instead
		AbilityWindow.DeleteControl (i+0x10000009)
		Button.SetText (AbilityTable.GetValue (i, 0))
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

		Button = AbilityWindow.GetControl(i*2+16)
		Button.OnPress (LeftPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

		Button = AbilityWindow.GetControl(i*2+17)
		Button.OnPress (RightPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

	TextAreaControl = AbilityWindow.GetControl(29)
	TextAreaControl.SetText(17247)

	StoreButton.OnPress (StorePress)
	RecallButton.OnPress (RecallPress)
	RerollButton.OnPress (RollPress)
	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	AbilityWindow.Focus()
	return

def RightPress(btn):
	global PointsLeft

	Abidx = btn.Value
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
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
	Ability = GemRB.GetVar("Ability "+str(Abidx) )
	CalcLimits(Abidx)
	GemRB.SetToken("MINIMUM",str(Minimum) )
	GemRB.SetToken("MAXIMUM",str(Maximum) )
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

	e=GemRB.GetVar("StoredStrExtra")
	GemRB.SetVar("StrExtra",e)
	Total = 0
	for i in range(-1,6):
		v = GemRB.GetVar("Stored "+str(i) )
		Total += v
		GemRB.SetVar("Ability "+str(i), v)
		Label = AbilityWindow.GetControl(0x10000003+i)
		if i==0 and v==18 and HasStrExtra==1:
			Label.SetText("18/"+str(e) )
		else:
			Label.SetText(str(v) )

	PointsLeft = GemRB.GetVar("Ability -1")

	# add a counter to the title
	SumLabel = AbilityWindow.GetControl (0x10000000)
	SumLabel.SetText(GemRB.GetString(11976) + ": " + str(Total))
	return

def BackPress():
	if AbilityWindow:
		AbilityWindow.Close ()
	GemRB.SetNextScript("CharGen5")
	GemRB.SetVar("StrExtra",0)
	for i in range(-1,6):
		GemRB.SetVar("Ability "+str(i),0)  #scrapping the abilities
	return

def NextPress():
	if AbilityWindow:
		AbilityWindow.Close ()
	AbilityTable = GemRB.LoadTable ("ability")
	AbilityCount = AbilityTable.GetRowCount ()

	for i in range (AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		StatValue = GemRB.GetVar ("Ability "+str(i))
		GemRB.SetPlayerStat (MyChar, StatID, StatValue)

	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))

	GemRB.SetNextScript("CharGen6")
	return

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
import CharGenCommon
import GUICommon
import GUICommonWindows
import CommonTables
import GameCheck
from ie_stats import *
from GUIDefines import *
from ie_restype import RES_2DA

AbilityWindow = 0
TextAreaControl = 0
DoneButton = 0
Minimum = 0
Maximum = 0
Add = 0
KitIndex = 0
HasStrExtra = 0
MyChar = 0

AbilityTable = GemRB.LoadTable ("ability")
Abracead = GemRB.LoadTable ("ABRACEAD")
Abclsmod = GemRB.LoadTable ("ABCLSMOD", True)
Abclasrq = GemRB.LoadTable ("ABCLASRQ")
Abracerq = GemRB.LoadTable ("ABRACERQ")

def CalcLimits(Abidx):
	global Minimum, Maximum, Add

	Race = CommonTables.Races.FindValue (3, GemRB.GetPlayerStat (MyChar, IE_RACE))
	RaceName = CommonTables.Races.GetRowName (Race)

	Minimum = 3
	Maximum = 18

	Race = Abracerq.GetRowIndex (RaceName)
	tmp = Abracerq.GetValue (Race, Abidx * 2)
	if tmp > Minimum:
		Minimum = tmp

	tmp = Abracerq.GetValue (Race, Abidx * 2 + 1)
	if tmp > Maximum:
		Maximum = tmp

	tmp = Abclasrq.GetValue (KitIndex, Abidx)
	if tmp > Minimum:
		Minimum = tmp

	Race = Abracead.GetRowIndex (RaceName)
	Add = Abracead.GetValue (Race, Abidx)
	Minimum += Add
	Maximum += Add

	if Abclsmod:
		tmp = Abclsmod.GetValue (KitIndex, Abidx)
		Maximum += tmp
		Add += tmp

	if Minimum < 1:
		Minimum = 1
	if Maximum > 25:
		Maximum = 25

	return

# add a counter to the title label
def UpdatePointsLabel (points):
	points = str(points)
	sumLabel = AbilityWindow.GetControl (0x10000000)
	sumLabel.SetText (GemRB.GetString (11976) + ": " + points)
	sumLabel2 = AbilityWindow.GetControl (0x10000000 + 40)
	if sumLabel2:
		sumLabel2.SetText (points)

# TODO: now we don't handle extra strength in steps — shouldn't it be the case (0/33/66/99/100)?
# in pst we go to great lengths to make it work
def SetAbilityScoreLabel (index, score, strExtra, mod = 0):
	label = AbilityWindow.GetControl (0x10000003 + index)
	if index == 0 and HasStrExtra == 1 and score == 18 - mod and (True if mod == 1 else strExtra > 0):
		# display 2 digits of exceptional strength, with 00 for 100
		label.SetText ("18/%02d" % (strExtra % 100))
	else:
		label.SetText (str(score + mod))

	# reusing logic here since both +, -, rolling and recalling needs it
	if GemRB.GetVar("Ability -1") == 0:
		DoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState (IE_GUI_BUTTON_DISABLED)

def RollPress():
	global Minimum, Maximum, Add, HasStrExtra
	global AllPoints18

	GemRB.SetVar ("Ability", 0)
	GemRB.SetVar ("Ability -1", 0) # holds the points available to distribute
	SumLabel = AbilityWindow.GetControl (0x10000002)
	SumLabel.SetText ("0")

	if HasStrExtra:
		if AllPoints18:
			e = 100
		else:
			e = GemRB.Roll (1, 100, 0)
	else:
		e = 0
	GemRB.SetVar ("StrExtra", e)

	dice = 3
	size = 6

	total = 0
	totMax = 108 # ensure the loop will complete
	# roll stats until total points are 75 or above
	while total < 75 and totMax >= 75:
		total = 0
		totMax = 0

		for i in range(6):
			CalcLimits (i)

			totMax += Maximum

			v = 0
			if AllPoints18:
				v = 18
			elif Maximum <= Minimum:
				# this is what the code used to do in this degenerate situation
				v = Maximum
			else:
				# roll until the result falls in the allowed range
				while v < Minimum or v > Maximum:
					v = GemRB.Roll (dice, size, Add)

			GemRB.SetVar ("Ability " + str(i), v)
			total += v
			SetAbilityScoreLabel (i, v, e)

	UpdatePointsLabel (total)

	AllPoints18 = 0
	return

def GiveAll18():
	global AllPoints18

	AllPoints18 = 1
	RollPress()

def OnLoad():
	global AbilityWindow, TextAreaControl, DoneButton
	global HasStrExtra
	global KitIndex, MyChar
	global AllPoints18

	AllPoints18 = 0

	MyChar = GemRB.GetVar ("Slot")
	Kit = GUICommon.GetKitIndex (MyChar)
	ClassName = GUICommon.GetClassRowName (MyChar)
	if Kit == 0:
		KitName = ClassName
	else:
		#rowname is just a number, first value row what we need here
		KitName = CommonTables.KitList.GetValue (Kit, 0)

	#if the class uses the warrior table for saves, then it may have the extra strength
	HasStrExtra = CommonTables.Classes.GetValue (ClassName, "STREXTRA", GTV_INT)

	KitIndex = Abclasrq.GetRowIndex (KitName)
	if KitIndex == None:
		KitIndex = 9999999 # for broken mods / installs; both tables have 0 defaults which work fine

	AbilityWindow = GemRB.LoadWindow (4, "GUICG")
	if GameCheck.IsBG2 ():
		CharGenCommon.PositionCharGenWin (AbilityWindow)

	Button = AbilityWindow.CreateButton (2000, 0, 0, 0, 0)
	Button.OnPress (GiveAll18)
	Button.SetHotKey ("8", GEM_MOD_CTRL, True)

	RerollButton = AbilityWindow.GetControl (2)
	RerollButton.SetText (11982)
	StoreButton = AbilityWindow.GetControl (37)
	StoreButton.SetText (17373)
	RecallButton = AbilityWindow.GetControl (38)
	RecallButton.SetText (17374)
	RecallButton.SetState (IE_GUI_BUTTON_DISABLED)

	BackButton = AbilityWindow.GetControl (36)
	BackButton.SetText (15416)
	BackButton.MakeEscape()
	DoneButton = AbilityWindow.GetControl (0)
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()

	RollPress()
	for i in range(6):
		Button = AbilityWindow.GetControl (i + 30)
		Button.OnPress (JustPress)
		if GameCheck.IsBG2 ():
			Button.OnMouseLeave (EmptyPress)
		Button.SetValue (i)
		# delete the labels and use the buttons instead
		if GameCheck.IsBG2 ():
			AbilityWindow.DeleteControl (i + 0x10000009)
			Button.SetText (AbilityTable.GetValue (i, 0))
			Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT, OP_OR)
			Button.SetState (IE_GUI_BUTTON_LOCKED)

		Button = AbilityWindow.GetControl (i * 2 + 16)
		Button.OnPress (PlusPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

		Button = AbilityWindow.GetControl (i * 2 + 17)
		Button.OnPress (MinusPress)
		Button.SetValue (i)
		Button.SetActionInterval (200)

	TextAreaControl = AbilityWindow.GetControl (29)
	TextAreaControl.SetText (17247)

	StoreButton.OnPress (StorePress)
	RecallButton.OnPress (RecallPress)
	RerollButton.OnPress (RollPress)
	DoneButton.OnPress (NextPress)
	if GameCheck.IsBG1 ():
		BackButton.OnPress (lambda: CharGenCommon.back(AbilityWindow))
		AbilityWindow.ShowModal (MODAL_SHADOW_GRAY)
	else:
		BackButton.OnPress (BackPress)
		AbilityWindow.Focus ()
	return

def MinusPress(Button):
	Abidx = Button.Value
	Ability = GemRB.GetVar ("Ability " + str(Abidx))
	CalcLimits (Abidx)
	GemRB.SetToken ("MINIMUM", str(Minimum))
	GemRB.SetToken ("MAXIMUM", str(Maximum))
	TextAreaControl.SetText (AbilityTable.GetValue (Abidx, 1))
	if Ability <= Minimum:
		return
	GemRB.SetVar ("Ability " + str(Abidx), Ability - 1)
	PointsLeft = GemRB.GetVar ("Ability -1") + 1
	GemRB.SetVar ("Ability -1", PointsLeft)
	SumLabel = AbilityWindow.GetControl (0x10000002)
	SumLabel.SetText (str(PointsLeft))
	SetAbilityScoreLabel (Abidx, Ability, GemRB.GetVar ("StrExtra"), -1)
	return

def JustPress(Button):
	Abidx = Button.Value
	CalcLimits (Abidx)
	GemRB.SetToken ("MINIMUM", str(Minimum))
	GemRB.SetToken ("MAXIMUM", str(Maximum))
	TextAreaControl.SetText (AbilityTable.GetValue (Abidx, 1))
	return

def PlusPress(Button):
	Abidx = Button.Value
	Ability = GemRB.GetVar ("Ability " + str(Abidx))
	CalcLimits (Abidx)
	GemRB.SetToken ("MINIMUM", str(Minimum))
	GemRB.SetToken ("MAXIMUM", str(Maximum))
	TextAreaControl.SetText (AbilityTable.GetValue (Abidx, 1))
	PointsLeft = GemRB.GetVar ("Ability -1") - 1
	if PointsLeft == -1:
		return
	if Ability >= Maximum:
		return
	GemRB.SetVar ("Ability " + str(Abidx), Ability + 1)
	GemRB.SetVar ("Ability -1", PointsLeft)
	SumLabel = AbilityWindow.GetControl (0x10000002)
	SumLabel.SetText (str(PointsLeft))
	SetAbilityScoreLabel (Abidx, Ability, GemRB.GetVar ("StrExtra"), 1)
	return

def EmptyPress():
	TextAreaControl = AbilityWindow.GetControl (29)
	TextAreaControl.SetText (17247)
	return

def StorePress():
	GemRB.SetVar ("StoredStrExtra", GemRB.GetVar ("StrExtra"))
	for i in range(-1, 6):
		GemRB.SetVar ("Stored " + str(i), GemRB.GetVar ("Ability " + str(i)))
	# enable recall
	AbilityWindow.GetControl (38).SetState (IE_GUI_BUTTON_ENABLED)
	return

def RecallPress():
	e = GemRB.GetVar ("StoredStrExtra")
	GemRB.SetVar ("StrExtra", e)
	Total = 0
	for i in range(-1, 6):
		v = GemRB.GetVar ("Stored " + str(i))
		Total += v
		GemRB.SetVar ("Ability " + str(i), v)
		SetAbilityScoreLabel (i, v, e)

	UpdatePointsLabel (Total)
	return

def BackPress():
	if AbilityWindow:
		AbilityWindow.Close ()
	GemRB.SetNextScript ("CharGen5")
	GemRB.SetVar ("StrExtra",0)
	for i in range(-1, 6):
		GemRB.SetVar ("Ability " + str(i), 0) #scrapping the abilities
	return

def NextPress():
	if AbilityWindow:
		AbilityWindow.Close ()

	AbilityCount = AbilityTable.GetRowCount ()
	for i in range (AbilityCount):
		StatID = AbilityTable.GetValue (i, 3)
		StatValue = GemRB.GetVar ("Ability " + str(i))
		GemRB.SetPlayerStat (MyChar, StatID, StatValue)

	GemRB.SetPlayerStat (MyChar, IE_STREXTRA, GemRB.GetVar ("StrExtra"))

	if GameCheck.IsBG1 ():
		CharGenCommon.next()
	else:
		GemRB.SetNextScript ("CharGen6")
	return

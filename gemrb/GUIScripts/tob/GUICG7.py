# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUICG7.py,v 1.10 2004/12/14 22:37:47 avenger_teambg Exp $
# character generation, mage spells (GUICG7)

import GemRB
from GUICommon import GetLearnableMageSpells, GetLearnablePriestSpells

MageSpellsWindow = 0
TextAreaControl = 0
DoneButton = 0
Learnable = []

def OnLoad():
	global MageSpellsWindow, TextAreaControl, DoneButton
	global MageSpellsSelectPointsLeft, Learnable
	
	AlignmentTable = GemRB.LoadTable("aligns")
	ClassTable = GemRB.LoadTable("classes")
	ClassRow = GemRB.GetVar("Class")-1
	Class = GemRB.GetTableValue(ClassTable, ClassRow, 5)
	TmpTable = GemRB.LoadTable("clskills")
	TableName = GemRB.GetTableValue(TmpTable, Class, 2)
	if TableName == "*":
		GemRB.SetNextScript("GUICG6")
		return

	GemRB.LoadWindowPack("GUICG")
	MageSpellsWindow = GemRB.LoadWindow(7)
	v = GemRB.GetVar("Alignment")
	Learnable = GetLearnableMageSpells( GemRB.GetVar("Class Kit"), v, 1)
	GemRB.SetVar("MageSpellBook", 0)
	GemRB.SetVar("SpellMask", 0)

	MageSpellsSelectPointsLeft = 2
	PointsLeftLabel = GemRB.GetControl(MageSpellsWindow, 0x1000001b)
	GemRB.SetLabelUseRGB(MageSpellsWindow, PointsLeftLabel, 1)
	GemRB.SetText(MageSpellsWindow, PointsLeftLabel, str(MageSpellsSelectPointsLeft))

	for i in range (24):
		SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
		GemRB.SetButtonFlags(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		if i < len(Learnable):
			Spell = GemRB.GetSpell(Learnable[i])
			GemRB.SetSpellIcon(MageSpellsWindow, SpellButton, Learnable[i])
			GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ENABLED)
			GemRB.SetEvent(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsSelectPress")
			GemRB.SetVarAssoc(MageSpellsWindow, SpellButton, "SpellMask", 1 << i)
			GemRB.SetTooltip(MageSpellsWindow, SpellButton, Spell['SpellName'])
		else:
			GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken("number", str(MageSpellsSelectPointsLeft))
	MageSpellsTextArea = GemRB.GetControl(MageSpellsWindow, 27)
	GemRB.SetText(MageSpellsWindow, MageSpellsTextArea, 17250)

	DoneButton = GemRB.GetControl(MageSpellsWindow, 0)
	GemRB.SetButtonState(MageSpellsWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent(MageSpellsWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsDonePress")
	GemRB.SetText(MageSpellsWindow, DoneButton, 11973)
	GemRB.SetButtonFlags(MageSpellsWindow, DoneButton, IE_GUI_BUTTON_DEFAULT, OP_OR)

	MageSpellsCancelButton = GemRB.GetControl(MageSpellsWindow, 29)
	GemRB.SetButtonState(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(MageSpellsWindow, MageSpellsCancelButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsCancelPress")
	GemRB.SetText(MageSpellsWindow, MageSpellsCancelButton, 13727)

	MageSpellsPickButton = GemRB.GetControl(MageSpellsWindow, 30)
	GemRB.SetButtonState(MageSpellsWindow, MageSpellsPickButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetEvent(MageSpellsWindow, MageSpellsPickButton, IE_GUI_BUTTON_ON_PRESS, "MageSpellsPickPress")
	GemRB.SetText(MageSpellsWindow, MageSpellsPickButton, 34210)

	GemRB.SetVisible(MageSpellsWindow,1)
	return

def MageSpellsSelectPress():
	global MageSpellsWindow, MageSpellsTextArea, DoneButton
	global MageSpellsSelectPointsLeft, Learnable

	MageSpellBook = GemRB.GetVar("MageSpellBook")
	SpellMask = GemRB.GetVar("SpellMask")
	Spell = abs(MageSpellBook - SpellMask)

	i = -1
	while (Spell > 0):
		i = i + 1
		Spell = Spell >> 1

	Spell = GemRB.GetSpell(Learnable[i])
	GemRB.SetText(MageSpellsWindow, MageSpellsTextArea, Spell["SpellDesc"])
	if SpellMask < MageSpellBook:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft + 1
		for i in range (len(Learnable)):
			SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
			if (((1 << i) & SpellMask) == 0):
				GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonState(MageSpellsWindow, DoneButton, IE_GUI_BUTTON_DISABLED)
	else:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft - 1
		if MageSpellsSelectPointsLeft == 0:
			for i in range (len(Learnable)):
				SpellButton = GemRB.GetControl(MageSpellsWindow, i + 2)
				if ((1 << i) & SpellMask) == 0:
					GemRB.SetButtonState(MageSpellsWindow, SpellButton, IE_GUI_BUTTON_DISABLED)
			GemRB.SetButtonState(MageSpellsWindow, DoneButton, IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = GemRB.GetControl(MageSpellsWindow, 0x1000001b)
	GemRB.SetText(MageSpellsWindow, PointsLeftLabel, str(MageSpellsSelectPointsLeft))
	GemRB.SetVar("MageSpellBook", SpellMask)
	return

def MageSpellsCancelPress():
	GemRB.UnloadWindow(MageSpellsWindow)
	GemRB.SetNextScript("CharGen6") #haterace
	return

def MageSpellsDonePress():
	GemRB.UnloadWindow(MageSpellsWindow)
	GemRB.SetNextScript("GUICG6") #abilities
	return

def MageSpellsPickPress():
	global MageSpellsSelectPointsLeft, Learnable

	SpellMask = 0
	Range = len(Learnable)
	if MageSpellsSelectPointsLeft > Range:
		MageSpellsSelectPointsLeft = Range
	if MageSpellsSelectPointsLeft:
		#make this less ugly without ruining Learnable
		for i in range(MageSpellsSelectPointsLeft):
			j = GemRB.Roll(1,Range,-1)
			if SpellMask & (1<<j):
				continue
			SpellMask = SpellMask | (1<<j)
		GemRB.SetVar("MageSpellBook", SpellMask)
	MageSpellsDonePress()
	return


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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$
# character generation, mage spells (GUICG7)

import GemRB
from GUICommon import GetLearnableMageSpells, GetLearnablePriestSpells

MageSpellsWindow = 0
MageSpellsTextArea = 0
DoneButton = 0
Learnable = []

def OnLoad():
	global MageSpellsWindow, MageSpellsTextArea, DoneButton
	global MageSpellsSelectPointsLeft, Learnable
	
	AlignmentTable = GemRB.LoadTableObject("aligns")
	ClassTable = GemRB.LoadTableObject("classes")
	ClassRow = GemRB.GetVar("Class")-1
	Class = ClassTable.GetValue(ClassRow, 5)
	TmpTable = GemRB.LoadTableObject("clskills")
	TableName = TmpTable.GetValue(Class, 2)
	if TableName == "*":
		GemRB.SetNextScript("GUICG6")
		return

	GemRB.LoadWindowPack("GUICG")
	MageSpellsWindow = GemRB.LoadWindowObject(7)
	v = GemRB.GetVar("Alignment")
	Learnable = GetLearnableMageSpells( GemRB.GetVar("Class Kit"), v, 1)
	GemRB.SetVar("MageSpellBook", 0)
	GemRB.SetVar("SpellMask", 0)

	MageSpellsSelectPointsLeft = 2
	PointsLeftLabel = MageSpellsWindow.GetControl(0x1000001b)
	PointsLeftLabel.SetUseRGB(1)
	PointsLeftLabel.SetText(str(MageSpellsSelectPointsLeft))

	for i in range (24):
		SpellButton = MageSpellsWindow.GetControl(i + 2)
		SpellButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_CHECKBOX, OP_OR)
		SpellButton.SetSprites("GUIBTBUT", 0, (i % 12) * 2, (i % 12) * 2 + 1, (i % 12) * 2 + 24, (i % 12) * 2 + 25)
		if i < len(Learnable):
			Spell = GemRB.GetSpell(Learnable[i])
			SpellButton.SetSpellIcon(Learnable[i], 1)
			SpellButton.SetState(IE_GUI_BUTTON_ENABLED)
			SpellButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MageSpellsSelectPress")
			SpellButton.SetVarAssoc("SpellMask", 1 << i)
			SpellButton.SetTooltip(Spell['SpellName'])
		else:
			SpellButton.SetState(IE_GUI_BUTTON_DISABLED)

	GemRB.SetToken("number", str(MageSpellsSelectPointsLeft))
	MageSpellsTextArea = MageSpellsWindow.GetControl(27)
	MageSpellsTextArea.SetText(17250)

	DoneButton = MageSpellsWindow.GetControl(0)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MageSpellsDonePress")
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

	MageSpellsCancelButton = MageSpellsWindow.GetControl(29)
	MageSpellsCancelButton.SetState(IE_GUI_BUTTON_ENABLED)
	MageSpellsCancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MageSpellsCancelPress")
	MageSpellsCancelButton.SetText(13727)

	MageSpellsWindow.SetVisible(1)
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
	MageSpellsTextArea.SetText(Spell["SpellDesc"])
	if SpellMask < MageSpellBook:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft + 1
		for i in range (len(Learnable)):
			SpellButton = MageSpellsWindow.GetControl(i + 2)
			if (((1 << i) & SpellMask) == 0):
				SpellButton.SetState(IE_GUI_BUTTON_ENABLED)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		MageSpellsSelectPointsLeft = MageSpellsSelectPointsLeft - 1
		if MageSpellsSelectPointsLeft == 0:
			for i in range (len(Learnable)):
				SpellButton = MageSpellsWindow.GetControl(i + 2)
				if ((1 << i) & SpellMask) == 0:
					SpellButton.SetState(IE_GUI_BUTTON_DISABLED)
			DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	PointsLeftLabel = MageSpellsWindow.GetControl(0x1000001b)
	PointsLeftLabel.SetText(str(MageSpellsSelectPointsLeft))
	GemRB.SetVar("MageSpellBook", SpellMask)
	return

def MageSpellsCancelPress():
	if MageSpellsWindow:
		MageSpellsWindow.Unload()
	GemRB.SetNextScript("CharGen6") #haterace
	return

def MageSpellsDonePress():
	if MageSpellsWindow:
		MageSpellsWindow.Unload()
	GemRB.SetNextScript("GUICG6") #abilities
	return


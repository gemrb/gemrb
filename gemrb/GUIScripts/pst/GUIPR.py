# -*-python-*-
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIPR.py - scripts to control priest spells windows from GUIPR winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_CAST

PriestWindow = None
PriestSpellInfoWindow = None
PriestSpellLevel = 0
PriestSpellUnmemorizeWindow = None


def InitPriestWindow (Window):
	global PriestSpellWindow
	PriestSpellWindow = Window

	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestPrevLevelPress)

	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestNextLevelPress)

	# Setup memorized spells buttons
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 160}
		Icon.SetBorder (0,  color,  0, 1)
		Icon.SetVarAssoc ("SpellButton", i)

	return

def UpdatePriestWindow (Window):
	global PriestMemorizedSpellList, PriestKnownSpellList

	PriestMemorizedSpellList = []
	PriestKnownSpellList = []

	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_PRIEST
	level = PriestSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)
	
	ClassName = GUICommon.GetClassRowName (pc)
	CantCast = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL") == "*"
	CantCast += GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	Name = GemRB.GetPlayerName (pc, 1)
	Label = Window.GetControl (0x10000027)
	Label.SetText (Name)

	Label = Window.GetControl (0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	Label.SetText (19672)


	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level, False)
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 160}
		Icon.SetBorder (0, color,  0, 1)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			Icon.SetSpellIcon (ms['SpellResRef'])
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenPriestSpellUnmemorizeWindow)
			else:
				Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnPriestUnmemorizeSpell)
			Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenPriestSpellInfoWindow)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Icon.SetTooltip (spell['SpellName'])
			PriestMemorizedSpellList.append (ms['SpellResRef'])
			Icon.EnableBorder (0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				Icon.SetSprites ("IVSLOT", 0,  0, 0, 0, 0)
			else:
				Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Icon.SetTooltip ('')
			Icon.EnableBorder (0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	btncount = 20
	for i in range (known_cnt):
		Icon = Window.GetControl (14 + i)
		ks = GemRB.GetKnownSpell (pc, type, level, i)
		Icon.SetSpellIcon (ks['SpellResRef'])
		Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnPriestMemorizeSpell)
		Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenPriestSpellInfoWindow)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		Icon.SetTooltip (spell['SpellName'])
		PriestKnownSpellList.append (ks['SpellResRef'])
		Icon.SetVarAssoc ("SpellButton", 100 + i)
			
	for i in range (i, btncount):
		Icon = Window.GetControl (14 + i)
		Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Icon.SetTooltip ('')

	return

TogglePriestWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIPR", GUICommonWindows.ToggleWindow, InitPriestWindow, UpdatePriestWindow, WINDOW_TOP|WINDOW_HCENTER)
OpenPriestWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIPR", GUICommonWindows.OpenWindowOnce, InitPriestWindow, UpdatePriestWindow, WINDOW_TOP|WINDOW_HCENTER)

def PriestPrevLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel > 0:
		PriestSpellLevel = PriestSpellLevel - 1
		UpdatePriestWindow (PriestSpellWindow)


def PriestNextLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel < 5:
		PriestSpellLevel = PriestSpellLevel + 1
		UpdatePriestWindow (PriestSpellWindow)


def OpenPriestSpellInfoWindow ():
	global PriestSpellInfoWindow

	if PriestSpellInfoWindow != None:
		if PriestSpellInfoWindow:
			PriestSpellInfoWindow.Unload ()
		PriestSpellInfoWindow = None

		return

	PriestSpellInfoWindow = Window = GemRB.LoadWindow (4)

	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenPriestSpellInfoWindow)

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = PriestMemorizedSpellList[index]
	else:
		ResRef = PriestKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Icon = Window.GetControl (1)
	Icon.SetSpellIcon (ResRef)

	Text = Window.GetControl (2)
	Text.SetText (spell['SpellDesc'])

	IconResRef = 'SPL' + spell['SpellbookIcon'][2:]

	Icon = Window.GetControl (5)
	Icon.SetSprites (IconResRef, 0, 0, 0, 0, 0)

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnPriestMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdatePriestWindow (PriestSpellWindow)

	# FIXME: use FLASH.bam


def OpenPriestSpellUnmemorizeWindow (btn, val):
	global PriestSpellUnmemorizeWindow

	if PriestSpellUnmemorizeWindow != None:
		PriestSpellUnmemorizeWindow.Unload ()
		PriestSpellUnmemorizeWindow = None
		return

	PriestSpellUnmemorizeWindow = Window = GemRB.LoadWindow (6)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (2)
	TextArea.SetText (50450)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (42514)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OnPriestUnmemorizeSpell(btn, val))
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenPriestSpellUnmemorizeWindow)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnPriestUnmemorizeSpell (btn, index):
	if PriestSpellUnmemorizeWindow:
		OpenPriestSpellUnmemorizeWindow (btn, index)

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdatePriestWindow (PriestSpellWindow)


###################################################
# End of file GUIPR.py

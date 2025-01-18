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
from GUICommon import BindControlCallbackParams
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_CAST

PriestWindow = None
PriestSpellLevel = 0

def InitPriestWindow (Window):
	global PriestSpellWindow
	PriestSpellWindow = Window

	Button = Window.GetControl (0)
	Button.OnPress (PriestPrevLevelPress)

	Button = Window.GetControl (1)
	Button.OnPress (PriestNextLevelPress)

	# Setup memorized spells buttons
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 160}
		Icon.SetBorder (0,  color,  0, 1)
		Icon.SetVarAssoc ("Memorized", i)

	return

def UpdatePriestWindow (Window):
	pc = GemRB.GameGetSelectedPCSingle ()
	spelltype = IE_SPELL_TYPE_PRIEST
	level = PriestSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, spelltype, level)

	GUICommon.AdjustWindowVisibility (Window, pc, GUICommon.CantUseSpellbookWindow (pc, True))

	Name = GemRB.GetPlayerName (pc, 1)
	Label = Window.GetControl (0x10000027)
	Label.SetText (Name)

	Label = Window.GetControl (0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	Label.SetText (19672)


	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 160}
		Icon.SetBorder (0, color,  0, 1)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, spelltype, level, i)
			Icon.SetSpellIcon (ms['SpellResRef'])
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				Icon.OnPress (OpenPriestSpellUnmemorizeWindow)
			else:
				Icon.OnPress (OnPriestUnmemorizeSpell)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Icon.OnRightPress (BindControlCallbackParams(OpenPriestSpellInfoWindow, spell))
			Icon.SetTooltip (spell['SpellName'])
			Icon.EnableBorder (0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				Icon.SetSprites ("IVSLOT", 0,  0, 0, 0, 0)
			else:
				Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Icon.OnPress (None)
			Icon.OnRightPress (None)
			Icon.SetTooltip ('')
			Icon.EnableBorder (0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, spelltype, level)
	btncount = 20
	i = 0
	for i in range (known_cnt):
		Icon = Window.GetControl (14 + i)
		ks = GemRB.GetKnownSpell (pc, spelltype, level, i)
		Icon.SetSpellIcon (ks['SpellResRef'])
		Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Icon.OnPress (OnPriestMemorizeSpell)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		Icon.OnRightPress (BindControlCallbackParams(OpenPriestSpellInfoWindow, spell))
		Icon.SetTooltip (spell['SpellName'])
		Icon.SetValue (i)

	if known_cnt == 0: i = -1
	for i in range (i + 1, btncount):
		Icon = Window.GetControl (14 + i)
		Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Icon.OnPress (None)
		Icon.OnRightPress (None)
		Icon.SetTooltip ('')

	return

TogglePriestWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIPR", GUICommonWindows.ToggleWindow, InitPriestWindow, UpdatePriestWindow)
OpenPriestWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIPR", GUICommonWindows.OpenWindowOnce, InitPriestWindow, UpdatePriestWindow)

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


def OpenPriestSpellInfoWindow (spell):
	Window = GemRB.LoadWindow (4, "GUIPR")

	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.OnPress (Window.Close)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])
	Label.SetFlags(IE_GUI_LABEL_USE_COLOR, OP_OR)

	Icon = Window.GetControl (1)
	Icon.SetSpellIcon (spell['SpellResRef'])

	Text = Window.GetControl (2)
	Text.SetText (spell['SpellDesc'])

	IconResRef = 'SPL' + spell['SpellbookIcon'][2:]

	Icon = Window.GetControl (5)
	Icon.SetSprites (IconResRef, 0, 0, 0, 0, 0)

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnPriestMemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	if GemRB.MemorizeSpell (pc, spelltype, level, btn.Value):
		UpdatePriestWindow (PriestSpellWindow)

	# FIXME: use FLASH.bam


def OpenPriestSpellUnmemorizeWindow (btn):
	Window = GemRB.LoadWindow (6, "GUIPR")

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (2)
	TextArea.SetText (50450)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (42514)
	def Unmemorize(btn):
		OnPriestUnmemorizeSpell(btn)
		Window.Close()

	Button.SetValue (btn.Value)
	Button.OnPress (Unmemorize)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnPriestUnmemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	if GemRB.UnmemorizeSpell (pc, spelltype, level, btn.Value):
		UpdatePriestWindow (PriestSpellWindow)


###################################################
# End of file GUIPR.py

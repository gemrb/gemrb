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


# GUIMG.py - scripts to control mage spells windows from GUIMG winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
from GUICommon import BindControlCallbackParams
from GUIDefines import *
from ie_stats import *

MageWindow = None
MageSpellLevel = 0

def InitMageWindow (Window):
	global MageWindow
	MageWindow = Window

	Button = Window.GetControl (0)
	Button.OnPress (MagePrevLevelPress)

	Button = Window.GetControl (1)
	Button.OnPress (MageNextLevelPress)

	# Setup memorized spells buttons
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 160}
		Icon.SetBorder (0, color,  0, 1)
		Icon.SetVarAssoc ("Memorized", i)

	for i in range (20):
		Icon = Window.GetControl (14 + i)
		Icon.SetValue (i)
	return

def UpdateMageWindow (Window=None):
	if Window == None:
		Window = MageWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	spelltype = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, spelltype, level)
	
#	CantCast = CommonTables.ClassSkills.GetValue (GUICommon.GetClassRowName (pc), "MAGESPELL") == "*"
#	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	Name = GemRB.GetPlayerName (pc, 1)
	Label = Window.GetControl (0x10000027)
	Label.SetText (Name)

	Label = Window.GetControl (0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	Label.SetText (19672)
	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, spelltype, level, i)
			Icon.SetSpellIcon (ms['SpellResRef'])
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				Icon.OnPress (OpenMageSpellUnmemorizeWindow)
			else:
				Icon.OnPress (OnMageUnmemorizeSpell)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Icon.OnRightPress (BindControlCallbackParams(OpenMageSpellInfoWindow, spell))
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
		Icon.OnPress (OnMageMemorizeSpell)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		Icon.OnRightPress (BindControlCallbackParams(OpenMageSpellInfoWindow, spell))
		Icon.SetTooltip (spell['SpellName'])

	if known_cnt == 0: i = -1
	for i in range (i + 1, btncount):
		Icon = Window.GetControl (14 + i)
		Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Icon.OnPress (None)
		Icon.OnRightPress (None)
		Icon.SetTooltip ('')
		

ToggleSpellWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMG", GUICommonWindows.ToggleWindow, InitMageWindow, UpdateMageWindow)
OpenSpellWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMG", GUICommonWindows.OpenWindowOnce, InitMageWindow, UpdateMageWindow)

def MagePrevLevelPress ():
	global MageSpellLevel

	if MageSpellLevel > 0:
		MageSpellLevel = MageSpellLevel - 1
		UpdateMageWindow ()


def MageNextLevelPress ():
	global MageSpellLevel

	if MageSpellLevel < 8:
		MageSpellLevel = MageSpellLevel + 1
		UpdateMageWindow ()

def OpenMageSpellInfoWindow (spell):
	Window = GemRB.LoadWindow (4, "GUIMG")

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


def OnMageMemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	if GemRB.MemorizeSpell (pc, spelltype, level, btn.Value):
		UpdateMageWindow ()

	# FIXME: use FLASH.bam


def OpenMageSpellUnmemorizeWindow (btn):
	Window = GemRB.LoadWindow (6, "GUIMG")

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (2)
	TextArea.SetText (50450)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (42514)
	def Unmemorize(btn):
		OnMageUnmemorizeSpell(btn)
		Window.Close()
	Button.SetValue (btn.Value)
	Button.OnPress (Unmemorize)
	Button.MakeDefault()

	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnMageUnmemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	spelltype = IE_SPELL_TYPE_WIZARD

	if GemRB.UnmemorizeSpell (pc, spelltype, level, btn.Value):
		UpdateMageWindow ()

###################################################
# End of file GUIMG.py

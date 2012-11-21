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


# GUIMG.py - scripts to control mage spells windows from GUIMG winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
from GUIDefines import *
from ie_stats import *

MageWindow = None
MageSpellInfoWindow = None
MageSpellLevel = 0
MageSpellUnmemorizeWindow = None

def OpenMageWindow ():
	global MageWindow

	if GUICommon.CloseOtherWindow (OpenMageWindow):
		GemRB.HideGUI ()
		if MageWindow:
			MageWindow.Unload ()
		MageWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GUICommonWindows.SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIMG")
	MageWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", MageWindow.ID)
	
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MagePrevLevelPress)

	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MageNextLevelPress)

	# Setup memorized spells buttons
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		Icon.SetBorder (0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)

	GUICommonWindows.SetSelectionChangeHandler (UpdateMageWindow)
	GemRB.UnhideGUI ()
	UpdateMageWindow ()

def UpdateMageWindow ():
	global MageMemorizedSpellList, MageKnownSpellList

	MageMemorizedSpellList = []
	MageKnownSpellList = []

	Window = MageWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)

	Name = GemRB.GetPlayerName (pc, 1)
	Label = Window.GetControl (0x10000027)
	Label.SetText (Name)

	Label = Window.GetControl (0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	Label.SetText (19672)
	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level, False)
	for i in range (12):
		Icon = Window.GetControl (2 + i)
		Icon.SetBorder (0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			Icon.SetSpellIcon (ms['SpellResRef'])
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellUnmemorizeWindow)
			else:
				Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageUnmemorizeSpell)
			Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenMageSpellInfoWindow)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Icon.SetTooltip (spell['SpellName'])
			MageMemorizedSpellList.append (ms['SpellResRef'])
			Icon.SetVarAssoc ("SpellButton", i)
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

	#--------------------------test-----------------------------#
	print "max_mem_cnt is: ", max_mem_cnt
	print "mem_cnt is:     ", mem_cnt
	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (20):
		Icon = Window.GetControl (14 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			Icon.SetSpellIcon (ks['SpellResRef'])
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageMemorizeSpell)
			Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenMageSpellInfoWindow)
			spell = GemRB.GetSpell (ks['SpellResRef'])
			Icon.SetTooltip (spell['SpellName'])
			MageKnownSpellList.append (ks['SpellResRef'])
			Icon.SetVarAssoc ("SpellButton", 100 + i)
		else:
			Icon.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Icon.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Icon.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Icon.SetTooltip ('')

	CantCast = CommonTables.ClassSkills.GetValue (GUICommon.GetClassRowName (pc), "MAGESPELL") == "*"
	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

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

def OpenMageSpellInfoWindow ():
	global MageSpellInfoWindow

	GemRB.HideGUI ()

	if MageSpellInfoWindow != None:
		if MageSpellInfoWindow:
			MageSpellInfoWindow.Unload ()
		MageSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	MageSpellInfoWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", MageSpellInfoWindow.ID)

	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellInfoWindow)

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = MageMemorizedSpellList[index]
	else:
		ResRef = MageKnownSpellList[index - 100]

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


	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnMageMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()

	# FIXME: use FLASH.bam


def OpenMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	GemRB.HideGUI ()

	if MageSpellUnmemorizeWindow != None:
		if MageSpellUnmemorizeWindow:
			MageSpellUnmemorizeWindow.Unload ()
		MageSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("FloatWindow", MageSpellUnmemorizeWindow.ID)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (2)
	TextArea.SetText (50450)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (42514)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnMageUnmemorizeSpell)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMageSpellUnmemorizeWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)


def OnMageUnmemorizeSpell ():
	if MageSpellUnmemorizeWindow:
		OpenMageSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()

###################################################
# End of file GUIMG.py

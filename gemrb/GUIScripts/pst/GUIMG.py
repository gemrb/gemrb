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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIMG.py,v 1.5 2004/11/15 00:20:54 edheldil Exp $


# GUIMG.py - scripts to control mage spells windows from GUIMG winpack

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import SetSelectionChangeHandler

MageWindow = None
MageSpellInfoWindow = None
MageSpellLevel = 0
MageSpellUnmemorizeWindow = None

def OpenMageWindow ():
	global MageWindow

	if CloseOtherWindow (OpenMageWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (MageWindow)
		MageWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIMG")
	MageWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", MageWindow)
	
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MagePrevLevelPress")

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MageNextLevelPress")

	# Setup memorized spells buttons
	for i in range (12):
		Icon = GemRB.GetControl (Window, 2 + i)
		GemRB.SetButtonBorder (Window, Icon, 0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)

	SetSelectionChangeHandler (UpdateMageWindow)
	UpdateMageWindow ()

	GemRB.UnhideGUI ()

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
	Label = GemRB.GetControl (Window, 0x10000027)
	GemRB.SetText (Window, Label, Name)

	Label = GemRB.GetControl (Window, 0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	GemRB.SetText (Window, Label, 19672)
	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (12):
		Icon = GemRB.GetControl (Window, 2 + i)
		GemRB.SetButtonBorder (Window, Icon, 0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Icon, ms['SpellResRef'])
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellUnmemorizeWindow")
			else:
				GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			GemRB.SetTooltip (Window, Icon, spell['SpellName'])
			MageMemorizedSpellList.append (ms['SpellResRef'])
			GemRB.SetVarAssoc (Window, Icon, "SpellButton", i)
			GemRB.EnableButtonBorder (Window, Icon, 0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				GemRB.SetButtonSprites (Window, Icon, "IVSLOT", 0,  0, 0, 0, 0)
			else:
				GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Icon, '')
			GemRB.EnableButtonBorder (Window, Icon, 0, 0)

	#--------------------------test-----------------------------#
	print "max_mem_cnt is: ", max_mem_cnt
	print "mem_cnt is:     ", mem_cnt
	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (20):
		Icon = GemRB.GetControl (Window, 14 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Icon, ks['SpellResRef'])
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_PRESS, "OnMageMemorizeSpell")
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Icon, spell['SpellName'])
			MageKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Icon, "SpellButton", 100 + i)
		else:
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Icon, '')


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
		GemRB.UnloadWindow (MageSpellInfoWindow)
		MageSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	MageSpellInfoWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", MageSpellInfoWindow)

	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellInfoWindow")

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = MageMemorizedSpellList[index]
	else:
		ResRef = MageKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, spell['SpellName'])

	Icon = GemRB.GetControl (Window, 1)
	GemRB.SetSpellIcon (Window, Icon, ResRef)

	Text = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Text, spell['SpellDesc'])

	IconResRef = 'SPL' + spell['SpellbookIcon'][2:]

	Icon = GemRB.GetControl (Window, 5)
	GemRB.SetButtonSprites (Window, Icon, IconResRef, 0, 0, 0, 0, 0)


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


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
		GemRB.UnloadWindow (MageSpellUnmemorizeWindow)
		MageSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("FloatWindow", MageSpellUnmemorizeWindow)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, TextArea, 50450)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 42514)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellUnmemorizeWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


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

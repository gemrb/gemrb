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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIPR.py,v 1.4 2004/09/01 18:32:32 edheldil Exp $


# GUIPR.py - scripts to control priest spells windows from GUIPR winpack

###################################################

import GemRB
from GUIDefines import *

#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
#import GUICommonWindows
from GUICommonWindows import SetSelectionChangeHandler

PriestWindow = None
PriestSpellInfoWindow = None
PriestSpellLevel = 0

def OpenPriestWindow ():
	global PriestWindow

	GemRB.HideGUI ()
	
	if PriestWindow != None:
		GemRB.UnloadWindow (PriestWindow)
		PriestWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIPR")
	PriestWindow = Window = GemRB.LoadWindow (3)
        GemRB.SetVar ("OtherWindow", PriestWindow)

	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestPrevLevelPress")

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestNextLevelPress")

	SetSelectionChangeHandler (UpdatePriestWindow)
	UpdatePriestWindow ()

	GemRB.UnhideGUI ()
	

def UpdatePriestWindow ():
	global PriestMemorizedSpellList, PriestKnownSpellList

	PriestMemorizedSpellList = []
	PriestKnownSpellList = []

	Window = PriestWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	Name = GemRB.GetPlayerName (pc, 0)
	Label = GemRB.GetControl (Window, 0x10000027)
	GemRB.SetText (Window, Label, Name)

	Label = GemRB.GetControl (Window, 0x10000026)
	GemRB.SetToken ('LEVEL', str (level + 1))
	GemRB.SetText (Window, Label, 19672)


	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (12):
		Icon = GemRB.GetControl (Window, 2 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Icon, ms['SpellResRef'])
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenPriestSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			GemRB.SetTooltip (Window, Icon, spell['SpellName'])
			PriestMemorizedSpellList.append (ms['SpellResRef'])
			GemRB.SetVarAssoc (Window, Icon, "SpellButton", i)
		else:
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Icon, '')


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (20):
		Icon = GemRB.GetControl (Window, 14 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Icon, ks['SpellResRef'])
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenPriestSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Icon, spell['SpellName'])
			PriestKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Icon, "SpellButton", 100 + i)
		else:
			GemRB.SetButtonFlags (Window, Icon, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Icon, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Icon, '')


def PriestPrevLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel > 0:
		PriestSpellLevel = PriestSpellLevel - 1
		UpdatePriestWindow ()


def PriestNextLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel < 8:
		PriestSpellLevel = PriestSpellLevel + 1
		UpdatePriestWindow ()


def OpenPriestSpellInfoWindow ():
	global PriestSpellInfoWindow

	GemRB.HideGUI ()
	
	if PriestSpellInfoWindow != None:
		GemRB.UnloadWindow (PriestSpellInfoWindow)
		PriestSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	PriestSpellInfoWindow = Window = GemRB.LoadWindow (4)
        GemRB.SetVar ("FloatWindow", PriestSpellInfoWindow)

	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellInfoWindow")

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = PriestMemorizedSpellList[index]
	else:
		ResRef = PriestKnownSpellList[index - 100]

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

###################################################
# End of file GUIPR.py

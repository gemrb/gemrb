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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg2/GUIPR.py,v 1.2 2004/09/03 23:23:34 avenger_teambg Exp $


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
PriestSpellUnmemorizeWindow = None


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
	PriestWindow = Window = GemRB.LoadWindow (2)
        GemRB.SetVar ("OtherWindow", PriestWindow)

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestPrevLevelPress")

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestNextLevelPress")

	#setup level buttons
	for i in range (7):
		Button = GemRB.GetControl (Window, 55 + i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshPriestLevel")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (7):
		Button = GemRB.GetControl (Window, 55 + i)
		GemRB.SetVarAssoc (Window, Button, "PriestSpellLevel", i)
		
	# Setup book spells buttons
	for i in range (12):
		Button = GemRB.GetControl (Window, 3 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_LOCKED, OP_OR)

	# Setup memorized spells buttons
	for i in range (12):
		Button = GemRB.GetControl (Window, 27 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_LOCKED, OP_OR)
		GemRB.SetButtonBorder (Window, Button, 0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)


	SetSelectionChangeHandler (UpdatePriestWindow)
	UpdatePriestWindow ()

	GemRB.UnhideGUI ()
	

def UpdatePriestWindow ():
	global PriestMemorizedSpellList, PriestKnownSpellList

	PriestMemorizedSpellList = []
	PriestKnownSpellList = []

	Window = PriestWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_PRIEST
	level = PriestSpellLevel
	# FIXME: how to calculate it???
	max_mem_cnt = 0

	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetText (Window, Label, GemRB.GetString(12137)+str(level+1) )

	Name = GemRB.GetPlayerName (pc, 0)
	Label = GemRB.GetControl (Window, 0x10000035)
	GemRB.SetText (Window, Label, Name)

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (12):
		Button = GemRB.GetControl (Window, 3 + i)
		GemRB.SetButtonBorder (Window, Button, 0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ms['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			if ms['Flags']:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellUnmemorizeWindow")
			else:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestUnmemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenPriestSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			PriestMemorizedSpellList.append (ms['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", i)
			GemRB.EnableButtonBorder (Window, Button, 0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				GemRB.SetButtonSprites (Window, Button, "IVSLOT", 0,  0, 0, 0, 0)
			else:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (12):
		Button = GemRB.GetControl (Window, 27 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ks['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestMemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenPriestSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			PriestKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", 100 + i)

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
	return


def PriestPrevLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel > 0:
		PriestSpellLevel = PriestSpellLevel - 1
		UpdatePriestWindow ()
	return


def PriestNextLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel < 6:
		PriestSpellLevel = PriestSpellLevel + 1
		UpdatePriestWindow ()
	return

def RefreshPriestLevel ():
	global PriestSpellLevel

	PriestSpellLevel = GemRB.GetVar ("PriestSpellLevel")
	UpdatePriestWindow ()
	return
	
def OpenPriestSpellInfoWindow ():
	global PriestSpellInfoWindow

	GemRB.HideGUI ()
	
	if PriestSpellInfoWindow != None:
		GemRB.UnloadWindow (PriestSpellInfoWindow)
		PriestSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	PriestSpellInfoWindow = Window = GemRB.LoadWindow (3)
        GemRB.SetVar ("FloatWindow", PriestSpellInfoWindow)

	Button = GemRB.GetControl (Window, 5)
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

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetSpellIcon (Window, Button, ResRef)

	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, spell['SpellDesc'])

	#IconResRef = 'SPL' + spell['SpellbookIcon'][2:]
	
	#Button = GemRB.GetControl (Window, 5)
	#GemRB.SetButtonSprites (Window, Button, IconResRef, 0, 0, 0, 0, 0)


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return


def OnPriestMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdatePriestWindow ()
	return


def OpenPriestSpellUnmemorizeWindow ():
	global PriestSpellUnmemorizeWindow
	
	GemRB.HideGUI ()
	
	if PriestSpellUnmemorizeWindow != None:
		GemRB.UnloadWindow (PriestSpellUnmemorizeWindow)
		PriestSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	PriestSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)
        GemRB.SetVar ("FloatWindow", PriestSpellUnmemorizeWindow)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, TextArea, 50450)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 42514)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestUnmemorizeSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellUnmemorizeWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return


def OnPriestUnmemorizeSpell ():
	if PriestSpellUnmemorizeWindow:
		OpenPriestSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdatePriestWindow ()
	return


###################################################
# End of file GUIPR.py

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
# $Id$


# GUIPR.py - scripts to control priest spells windows from GUIPR winpack

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

PriestWindow = None
PriestSpellInfoWindow = None
PriestSpellLevel = 0
PriestSpellUnmemorizeWindow = None
OldOptionsWindow = None


def OpenPriestWindow ():
	global PriestWindow, OptionsWindow
	global OldOptionsWindow


	if CloseOtherWindow (OpenPriestWindow):
		GemRB.UnloadWindow (PriestWindow)
		GemRB.UnloadWindow (OptionsWindow)
		PriestWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	
	GemRB.LoadWindowPack ("GUIPR")
	PriestWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", PriestWindow)
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenPriestWindow")
	GemRB.SetWindowFrame (OptionsWindow)

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestPrevLevelPress")

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PriestNextLevelPress")

## 	#setup level buttons
## 	for i in range (7):
## 		Button = GemRB.GetControl (Window, 55 + i)
## 		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshPriestLevel")
## 		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

## 	for i in range (7):
## 		Button = GemRB.GetControl (Window, 55 + i)
## 		GemRB.SetVarAssoc (Window, Button, "PriestSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (12):
		Button = GemRB.GetControl (Window, 3 + i)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,0,64,0,1)
		GemRB.SetButtonSprites (Window, Button, "SPELFRAM",0,0,0,0,0)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (20):
		Button = GemRB.GetControl (Window, 27 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	SetSelectionChangeHandler (UpdatePriestWindow)
	UpdatePriestWindow ()

	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (GUICommonWindows.PortraitWindow, 1)
	return


def UpdatePriestWindow ():
	global PriestMemorizedSpellList, PriestKnownSpellList

	PriestMemorizedSpellList = []
	PriestKnownSpellList = []

	Window = PriestWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	Type = IE_SPELL_TYPE_PRIEST
	level = PriestSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, Type, level)

	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetToken ('LEVEL', str (level + 1))
	GemRB.SetText (Window, Label, 12137)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = GemRB.GetControl (Window, 0x10000035)
	GemRB.SetText (Window, Label, Name)

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, Type, level)
	for i in range (12):
		Button = GemRB.GetControl (Window, 3 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, Type, level, i)
			GemRB.SetSpellIcon (Window, Button, ms['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
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
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NORMAL, OP_SET)
			else:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, Type, level)
	for i in range (20):
		Button = GemRB.GetControl (Window, 27 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, Type, level, i)
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
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)

	Table = GemRB.LoadTable ("clskills")
	#if (GemRB.GetTableValue (Table, GemRB.GetPlayerStat( GemRB.GameGetSelectedPCSingle(), IE_CLASS), 1)=="*"):
	#	GemRB.SetVisible (Window, 2)
	#else:
	GemRB.SetVisible (Window, 1)
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

	if PriestSpellInfoWindow != None:
		GemRB.UnloadWindow (PriestSpellInfoWindow)
		PriestSpellInfoWindow = None
		return

	PriestSpellInfoWindow = Window = GemRB.LoadWindow (3)

	#back
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 15416)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellInfoWindow")

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = PriestMemorizedSpellList[index]
	else:
		ResRef = PriestKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	#Label = GemRB.GetControl (Window, 0x0fffffff)
	#GemRB.SetText (Window, Label, spell['SpellName'])

	Label = GemRB.GetControl (Window, 0x10000000)
	GemRB.SetText (Window, Label, spell['SpellName'])

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetSpellIcon (Window, Button, ResRef)

	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, spell['SpellDesc'])

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OnPriestMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	Type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, Type, level, index):
		UpdatePriestWindow ()
	return

def OpenPriestSpellRemoveWindow ():
	global PriestSpellUnmemorizeWindow

	PriestSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 11824)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestRemoveSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellRemoveWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def ClosePriestSpellUnmemorizeWindow ():
	global PriestSpellUnmemorizeWindow

	GemRB.UnloadWindow (PriestSpellUnmemorizeWindow)
	PriestSpellUnmemorizeWindow = None
	return

def OpenPriestSpellUnmemorizeWindow ():
	global PriestSpellUnmemorizeWindow

	PriestSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 11824)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestUnmemorizeSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ClosePriestSpellUnmemorizeWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OnPriestUnmemorizeSpell ():
	if PriestSpellUnmemorizeWindow:
		ClosePriestSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	Type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, Type, level, index):
		UpdatePriestWindow ()
	return

def OnPriestRemoveSpell ():
	ClosePriestSpellUnmemorizeWindow()
	OpenPriestSpellInfoWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	Type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")-100

	#remove spell from memory
	GemRB.RemoveSpell (pc, Type, level, index)
	UpdatePriestWindow ()
	return

###################################################
# End of file GUIPR.py

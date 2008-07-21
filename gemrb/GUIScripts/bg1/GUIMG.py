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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUIMG.py - scripts to control mage spells windows from GUIMG winpack

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

MageWindow = None
MageSpellInfoWindow = None
MageSpellLevel = 0
MageSpellUnmemorizeWindow = None
OldOptionsWindow = None


def OpenMageWindow ():
	global MageWindow, OptionsWindow, PortraitWindow
	global OldOptionsWindow

	if CloseOtherWindow (OpenMageWindow):
		GemRB.UnloadWindow (MageWindow)
		GemRB.UnloadWindow (OptionsWindow)
		MageWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return
		
	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	GemRB.LoadWindowPack ("GUIMG")
	MageWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MageWindow)
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenMageWindow")
	GemRB.SetWindowFrame (OptionsWindow)
	
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MagePrevLevelPress")

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MageNextLevelPress")

## 	#unknown usage
## 	Button = GemRB.GetControl (Window, 55)
## 	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
## 	#GemRB.SetText (Window, Button, 123)
## 	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "xxPress")

## 	#setup level buttons
## 	for i in range (9):
## 		Button = GemRB.GetControl (Window, 56 + i)
## 		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshMageLevel")
## 		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

## 	for i in range (9):
## 		Button = GemRB.GetControl (Window, 56 + i)
## 		GemRB.SetVarAssoc (Window, Button, "MageSpellLevel", i)

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

	SetSelectionChangeHandler (UpdateMageWindow)
	UpdateMageWindow ()

	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return


def UpdateMageWindow ():
	global MageMemorizedSpellList, MageKnownSpellList

	MageMemorizedSpellList = []
	MageKnownSpellList = []

	Window = MageWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)
	
	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetToken ('LEVEL', str (level + 1))
	GemRB.SetText (Window, Label, 12137 )

	Name = GemRB.GetPlayerName (pc, 0)
	Label = GemRB.GetControl (Window, 0x10000035)
	GemRB.SetText (Window, Label, Name)

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (12):
		Button = GemRB.GetControl (Window, 3 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ms['SpellResRef'], 0)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if ms['Flags']:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellUnmemorizeWindow")
			else:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			MageMemorizedSpellList.append (ms['SpellResRef'])
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


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (20):
		Button = GemRB.GetControl (Window, 27 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ks['SpellResRef'], 0)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnMageMemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			MageKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", 100 + i)

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)

	Table = GemRB.LoadTable ("clskills")
	if (GemRB.GetTableValue (Table, GemRB.GetPlayerStat( GemRB.GameGetSelectedPCSingle(), IE_CLASS), 2)=="*"):
		GemRB.SetVisible (Window, 2)
	else:
		GemRB.SetVisible (Window, 1)
	return

def MagePrevLevelPress ():
	global MageSpellLevel

	if MageSpellLevel > 0:
		MageSpellLevel = MageSpellLevel - 1
		UpdateMageWindow ()
	return

def MageNextLevelPress ():
	global MageSpellLevel

	if MageSpellLevel < 8:
		MageSpellLevel = MageSpellLevel + 1
		UpdateMageWindow ()
	return

def RefreshMageLevel ():
	global MageSpellLevel

	MageSpellLevel = GemRB.GetVar ("MageSpellLevel")
	UpdateMageWindow ()
	return

def OnMageMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()
	return

def OpenMageSpellInfoWindow ():
	global MageSpellInfoWindow

	if MageSpellInfoWindow != None:
		GemRB.UnloadWindow (MageSpellInfoWindow)
		MageSpellInfoWindow = None
		return
		
	MageSpellInfoWindow = Window = GemRB.LoadWindow (3)

	#back
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 15416)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageSpellInfoWindow")

	#erase
	#Button = GemRB.GetControl (Window, 6)
	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = MageMemorizedSpellList[index]
	else:
		ResRef = MageKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, spell['SpellName'])

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetSpellIcon (Window, Button, ResRef, 1)

	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, spell['SpellDesc'])

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OnMageMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()
	return

def CloseMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	GemRB.UnloadWindow (MageSpellUnmemorizeWindow)
	MageSpellUnmemorizeWindow = None
	return

#def OpenMageSpellRemoveWindow ():
#	global MageSpellUnmemorizeWindow
#		
#	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)
#
#	# "Are you sure you want to ....?"
#	TextArea = GemRB.GetControl (Window, 3)
#	GemRB.SetText (Window, TextArea, 63745)
#
#	# Remove
#	Button = GemRB.GetControl (Window, 0)
#	GemRB.SetText (Window, Button, 17507)
#	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnMageRemoveSpell")
#
#	# Cancel
#	Button = GemRB.GetControl (Window, 1)
#	GemRB.SetText (Window, Button, 13727)
#	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseMageSpellUnmemorizeWindow")
#
#	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
#	return

def OpenMageSpellUnmemorizeWindow ():
	global MageSpellUnmemorizeWindow

	MageSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 11824)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnMageUnmemorizeSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseMageSpellUnmemorizeWindow")

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OnMageUnmemorizeSpell ():
	if MageSpellUnmemorizeWindow:
		CloseMageSpellUnmemorizeWindow()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = MageSpellLevel
	type = IE_SPELL_TYPE_WIZARD

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdateMageWindow ()
	return

#def OnMageRemoveSpell ():
#	CloseMageSpellUnmemorizeWindow()
#	OpenMageSpellInfoWindow()
#
#	pc = GemRB.GameGetSelectedPCSingle ()
#	level = MageSpellLevel
#	type = IE_SPELL_TYPE_WIZARD
#
#	index = GemRB.GetVar ("SpellButton")-100
#
#	#remove spell from book
#	GemRB.RemoveSpell (pc, type, level, index)
#	UpdateMageWindow ()
#	return

###################################################
# End of file GUIMG.py

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
PortraitWindow = None
OptionsWindow = None
OldPortraitWindow = None
OldOptionsWindow = None


def OpenMageWindow ():
	global MageWindow, PortraitWindow, OptionsWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenMageWindow):
		GemRB.UnloadWindow (MageWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

		MageWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return
		
	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	GemRB.LoadWindowPack ("GUIMG", 640, 480)
	MageWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MageWindow)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenMageWindow")
	GemRB.SetWindowFrame (OptionsWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)

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
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,0,160,0,1)
		GemRB.SetButtonBAM (Window, Button, "SPELFRAM",0,0,0)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (20):
		Button = GemRB.GetControl (Window, 27 + i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	SetSelectionChangeHandler (UpdateMageWindow)
	UpdateMageWindow ()
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 3)
	GemRB.SetVisible (PortraitWindow, 1)


def UpdateMageWindow ():
	global MageMemorizedSpellList, MageKnownSpellList

	MageMemorizedSpellList = []
	MageKnownSpellList = []

	Window = MageWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_WIZARD
	level = MageSpellLevel
	#max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)
	max_mem_cnt = 0
	
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
			GemRB.SetSpellIcon (Window, Button, ms['SpellResRef'])
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
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,
"")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (20):
		Button = GemRB.GetControl (Window, 27 + i)
		if i < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ks['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,
"OnMageMemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenMageSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			MageKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", 100 +
i)

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS,
"")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)
	Table = GemRB.LoadTable ("clskills")
	if (GemRB.GetTableValue (Table, GemRB.GetPlayerStat( GemRB.GameGetSelectedPCSingle(), IE_CLASS), 2)=="*"):
		GemRB.SetVisible (Window, 2)
	else:
		GemRB.SetVisible (Window, 1)


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


def RefreshMageLevel ():
	global MageSpellLevel

	MageSpellLevel = GemRB.GetVar ("MageSpellLevel")
	UpdateMageWindow ()


###################################################
# End of file GUIMG.py

# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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


# GUISPL.py - scripts to control priest spells windows from GUIPR winpack

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

TopIndex = 0
SpellBookWindow = None
SpellBookSpellInfoWindow = None
SpellBookSpellLevel = 0
SpellBookSpellUnmemorizeWindow = None
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None

def OpenSpellBookWindow ():
	global SpellBookWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow, TopIndex

	if CloseOtherWindow (OpenSpellBookWindow):
		GemRB.UnloadWindow (SpellBookWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

		SpellBookWindow = None
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

	GemRB.LoadWindowPack ("GUISPL", 800, 600)
	SpellBookWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", SpellBookWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenSpellBookWindow")
	GemRB.SetWindowFrame (Window)

	Button = GemRB.GetControl (Window, 92)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SpellBookPrevPress")

	Button = GemRB.GetControl (Window, 93)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SpellBookNextPress")

	#setup level buttons
	for i in range (9):
		Button = GemRB.GetControl (Window, 55 + i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshSpellBookLevel")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (9):
		Button = GemRB.GetControl (Window, 55 + i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		GemRB.SetVarAssoc (Window, Button, "SpellBookSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (24):
		Button = GemRB.GetControl (Window, 6 + i)
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,0,160,0,1)
		#GemRB.SetButtonBAM (Window, Button, "SPELFRAM",0,0,0)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (8):
		Button = GemRB.GetControl (Window, 30 + i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		GemRB.SetVarAssoc (Window, Button, "SpellIndex", i)

	ScrollBar = GemRB.GetControl (Window, 54)
	TopIndex = 0
	GemRB.SetVar ("TopIndex",0)
	GemRB.SetVarAssoc (Window, ScrollBar, "TopIndex", 0)

	SetSelectionChangeHandler (UpdateSpellBookWindow)
	UpdateSpellBookWindow ()
	return

def UpdateSpellBookWindow ():
	global SpellBookMemorizedSpellList, PriestKnownSpellList, TopIndex

	SpellBookMemorizedSpellList = []
	SpellBookKnownSpellList = []

	Window = SpellBookWindow
	pc = GemRB.GameGetSelectedPCSingle ()
	type = IE_SPELL_TYPE_PRIEST
	level = SpellBookSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)

	#Label = GemRB.GetControl (Window, 0x10000032)
	#GemRB.SetText (Window, Label, GemRB.GetString(12137)+str(level+1) )

	Name = GemRB.GetPlayerName (pc, 0)
	#Label = GemRB.GetControl (Window, 0xffffffff)
	#GemRB.SetText (Window, Label, Name)

	Button = GemRB.GetControl (Window, 1)
	GemRB.SetButtonPicture (Window, Button, GemRB.GetPlayerPortrait (pc,0))

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (24):
		Button = GemRB.GetControl (Window, 6 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			GemRB.SetSpellIcon (Window, Button, ms['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			if ms['Flags']:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellUnmemorizeWindow")
			else:
				GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnSpellBookUnmemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenSpellBookSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			SpellBookMemorizedSpellList.append (ms['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", i)
			GemRB.EnableButtonBorder (Window, Button, 0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (8):
		Button = GemRB.GetControl (Window, 30 + i)
		if i+TopIndex < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i+TopIndex)
			GemRB.SetSpellIcon (Window, Button, ks['SpellResRef'])
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnSpellBookMemorizeSpell")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenSpellBookSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			GemRB.SetTooltip (Window, Button, spell['SpellName'])
			SpellBookKnownSpellList.append (ks['SpellResRef'])
			GemRB.SetVarAssoc (Window, Button, "SpellButton", 100 + i)

		else:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)

	#if actor is uncontrollable, make this grayed
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	GemRB.SetVisible (OptionsWindow, 1)
	return

#TODO: spell type selector
def SpellBookPrevPress ():
	global SpellType

	UpdateSpellBookWindow ()
	return

#TODO: spell type selector
def SpellBookNextPress ():
	global SpellBookSpellLevel

	UpdateSpellBookWindow ()
	return

def RefreshSpellBookLevel ():
	global SpellBookSpellLevel

	SpellBookSpellLevel = GemRB.GetVar ("PriestSpellLevel")
	UpdateSpellBookWindow ()
	return

def OpenSpellBookSpellInfoWindow ():
	global SpellBookSpellInfoWindow

	GemRB.HideGUI ()

	if SpellBookSpellInfoWindow != None:
		GemRB.UnloadWindow (SpellBookSpellInfoWindow)
		SpellBookSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellInfoWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", SpellBookSpellInfoWindow)

	#back
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 15416)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellInfoWindow")

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = SpellBookMemorizedSpellList[index]
	else:
		ResRef = SpellBookKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = GemRB.GetControl (Window, 0x0fffffff)
	GemRB.SetText (Window, Label, spell['SpellName'])

	Button = GemRB.GetControl (Window, 2)
	GemRB.SetSpellIcon (Window, Button, ResRef)

	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, spell['SpellDesc'])

	#IconResRef = 'SPL' + spell['SpellBookIcon'][2:]

	#Button = GemRB.GetControl (Window, 5)
	#GemRB.SetButtonSprites (Window, Button, IconResRef, 0, 0, 0, 0, 0)


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return


def OnSpellBookMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100 + TopIndex

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdateSpellBookWindow ()
	return

def OpenSpellBookSpellRemoveWindow ():
	global SpellBookSpellUnmemorizeWindow

	GemRB.HideGUI ()

	if SpellBookSpellUnmemorizeWindow != None:
		GemRB.UnloadWindow (SpellBookSpellUnmemorizeWindow)
		SpellBookSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", SpellBookSpellUnmemorizeWindow)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 63745)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnSpellBookRemoveSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellRemoveWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OpenSpellBookSpellUnmemorizeWindow ():
	global SpellBookSpellUnmemorizeWindow

	GemRB.HideGUI ()

	if SpellBookSpellUnmemorizeWindow != None:
		GemRB.UnloadWindow (SpellBookSpellUnmemorizeWindow)
		SpellBookSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", SpellBookSpellUnmemorizeWindow)

	# "Are you sure you want to ....?"
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 11824)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnSpellBookUnmemorizeSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellUnmemorizeWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def OnSpellBookUnmemorizeSpell ():
	if SpellBookSpellUnmemorizeWindow:
		OpenSpellBookSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdateSpellBookWindow ()
	return


def OnSpellBookRemoveSpell ():
	if SpellBookSpellUnmemorizeWindow:
		OpenSpellBookSpellRemoveWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = SpellBookSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	#remove spell from memory
	#GemRB.UnmemorizeSpell (pc, type, level, index)
	#remove spell from book
	#GemRB.RemoveSpell (pc, type, level, index)
	UpdateSpellBookWindow ()
	return

###################################################
# End of file GUISPL.py

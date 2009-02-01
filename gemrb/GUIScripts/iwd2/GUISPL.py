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
		if SpellBookWindow:
			SpellBookWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

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
	SpellBookWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", SpellBookWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenSpellBookWindow")
	Window.SetFrame ()

	Button = Window.GetControl (92)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SpellBookPrevPress")

	Button = Window.GetControl (93)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SpellBookNextPress")

	#setup level buttons
	for i in range (9):
		Button = Window.GetControl (55 + i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RefreshSpellBookLevel")
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

	for i in range (9):
		Button = Window.GetControl (55 + i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetVarAssoc ("SpellBookSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (24):
		Button = Window.GetControl (6 + i)
		Button.SetBorder (0,0,0,0,0,0,0,0,160,0,1)
		#Button.SetBAM ("SPELFRAM",0,0,0)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (8):
		Button = Window.GetControl (30 + i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("SpellIndex", i)

	ScrollBar = Window.GetControl (54)
	TopIndex = 0
	GemRB.SetVar ("TopIndex",0)
	ScrollBar.SetVarAssoc ("TopIndex", 0)

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

	#Label = Window.GetControl (0x10000032)
	#Label.SetText (GemRB.GetString(12137)+str(level+1) )

	Name = GemRB.GetPlayerName (pc, 0)
	#Label = Window.GetControl (0xffffffff)
	#Label.SetText (Name)

	Button = Window.GetControl (1)
	Button.SetPicture (GemRB.GetPlayerPortrait (pc,0))

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
	for i in range (24):
		Button = Window.GetControl (6 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, type, level, i)
			Button.SetSpellIcon (ms['SpellResRef'])
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			if ms['Flags']:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellUnmemorizeWindow")
			else:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnSpellBookUnmemorizeSpell")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenSpellBookSpellInfoWindow")
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Button.SetTooltip (spell['SpellName'])
			SpellBookMemorizedSpellList.append (ms['SpellResRef'])
			Button.SetVarAssoc ("SpellButton", i)
			Button.EnableBorder (0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
				Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			Button.SetTooltip ('')
			Button.EnableBorder (0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (8):
		Button = Window.GetControl (30 + i)
		if i+TopIndex < known_cnt:
			ks = GemRB.GetKnownSpell (pc, type, level, i+TopIndex)
			Button.SetSpellIcon (ks['SpellResRef'])
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnSpellBookMemorizeSpell")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenSpellBookSpellInfoWindow")
			spell = GemRB.GetSpell (ks['SpellResRef'])
			Button.SetTooltip (spell['SpellName'])
			SpellBookKnownSpellList.append (ks['SpellResRef'])
			Button.SetVarAssoc ("SpellButton", 100 + i)

		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			Button.SetTooltip ('')
			Button.EnableBorder (0, 0)

	#if actor is uncontrollable, make this grayed
	Window.SetVisible (1)
	PortraitWindow.SetVisible (1)
	OptionsWindow.SetVisible (1)
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
		if SpellBookSpellInfoWindow:
			SpellBookSpellInfoWindow.Unload ()
		SpellBookSpellInfoWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellInfoWindow = Window = GemRB.LoadWindowObject (3)
	GemRB.SetVar ("FloatWindow", SpellBookSpellInfoWindow.ID)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellInfoWindow")

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = SpellBookMemorizedSpellList[index]
	else:
		ResRef = SpellBookKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	Label = Window.GetControl (0x0fffffff)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	#IconResRef = 'SPL' + spell['SpellBookIcon'][2:]

	#Button = Window.GetControl (5)
	#Button.SetSprites (IconResRef, 0, 0, 0, 0, 0)


	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
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
		if SpellBookSpellUnmemorizeWindow:
			SpellBookSpellUnmemorizeWindow.Unload ()
		SpellBookSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindowObject (5)
	GemRB.SetVar ("FloatWindow", SpellBookSpellUnmemorizeWindow.ID)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (63745)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnSpellBookRemoveSpell")

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellRemoveWindow")

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenSpellBookSpellUnmemorizeWindow ():
	global SpellBookSpellUnmemorizeWindow

	GemRB.HideGUI ()

	if SpellBookSpellUnmemorizeWindow != None:
		if SpellBookSpellUnmemorizeWindow:
			SpellBookSpellUnmemorizeWindow.Unload ()
		SpellBookSpellUnmemorizeWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return

	SpellBookSpellUnmemorizeWindow = Window = GemRB.LoadWindowObject (5)
	GemRB.SetVar ("FloatWindow", SpellBookSpellUnmemorizeWindow.ID)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnSpellBookUnmemorizeSpell")

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookSpellUnmemorizeWindow")

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
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

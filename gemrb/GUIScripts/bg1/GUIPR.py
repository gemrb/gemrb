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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg1/GUIPR.py,v 1.1 2004/12/04 12:33:42 avenger_teambg Exp $


# GUIPR.py - scripts to control priest spells windows from GUIPR winpack

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import SetSelectionChangeHandler

PriestWindow = None
PriestSpellInfoWindow = None
PriestSpellLevel = 0
PriestSpellUnmemorizeWindow = None


def OpenPriestWindow ():
	global PriestWindow

	
	if CloseOtherWindow (OpenPriestWindow):
		GemRB.HideGUI ()
		GemRB.UnloadWindow (PriestWindow)
		PriestWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		SetSelectionChangeHandler (None)
		GemRB.UnhideGUI ()
		return
		
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIPR")
	PriestWindow = Window = GemRB.LoadWindow (2)
        GemRB.SetVar ("OtherWindow", PriestWindow)

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
		GemRB.SetButtonBorder (Window, Button, 0,0,0,0,0,0,0,0,160,0,1)
		GemRB.SetButtonBAM (Window, Button, "SPELFRAM",0,0,0)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Setup book spells buttons
	for i in range (24):
		Button = GemRB.GetControl (Window, 27 + i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

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
	###max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, type, level)
	max_mem_cnt = 2
	
	Label = GemRB.GetControl (Window, 0x10000032)
	GemRB.SetToken ('LEVEL', str (level + 1))
	GemRB.SetText (Window, Label, 12137)

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
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
			else:
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
				GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


	known_cnt = GemRB.GetKnownSpellsCount (pc, type, level)
	for i in range (24):
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
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
			GemRB.SetTooltip (Window, Button, '')
			GemRB.EnableButtonBorder (Window, Button, 0, 0)


def PriestPrevLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel > 0:
		PriestSpellLevel = PriestSpellLevel - 1
		UpdatePriestWindow ()


def PriestNextLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel < 6:
		PriestSpellLevel = PriestSpellLevel + 1
		UpdatePriestWindow ()


def RefreshPriestLevel ():
	global PriestSpellLevel

	PriestSpellLevel = GemRB.GetVar ("PriestSpellLevel")
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
		
	PriestSpellInfoWindow = Window = GemRB.LoadWindow (3)
        GemRB.SetVar ("FloatWindow", PriestSpellInfoWindow)

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

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def OnPriestMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, type, level, index):
		UpdatePriestWindow ()


def OpenPriestSpellRemoveWindow ():
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
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 63745)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestRemoveSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellRemoveWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


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
	TextArea = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, TextArea, 11824)

	# Remove
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 17507)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OnPriestUnmemorizeSpell")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestSpellUnmemorizeWindow")

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def OnPriestUnmemorizeSpell ():
	if PriestSpellUnmemorizeWindow:
		OpenPriestSpellUnmemorizeWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	if GemRB.UnmemorizeSpell (pc, type, level, index):
		UpdatePriestWindow ()


def OnPriestRemoveSpell ():
	if PriestSpellUnmemorizeWindow:
		OpenPriestSpellRemoveWindow ()

	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	type = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton")

	#remove spell from memory
	#GemRB.UnmemorizeSpell (pc, type, level, index)
	#remove spell from book
	#GemRB.RemoveSpell (pc, type, level, index)
	UpdatePriestWindow ()


###################################################
# End of file GUIPR.py

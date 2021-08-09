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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIPR.py - scripts to control the priest spells windows from the GUIPR winpack

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
import CommonTables
from GUIDefines import *
from ie_stats import *
from ie_action import ACT_CAST

PriestSpellWindow = None
PriestSpellLevel = 0

def InitPriestWindow (Window):
	global PriestSpellWindow
	PriestSpellWindow = Window
	
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestPrevLevelPress)

	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PriestNextLevelPress)

	#setup level buttons
	if GameCheck.IsBG2():
		for i in range (7):
			Button = Window.GetControl (55 + i)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RefreshPriestLevel)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

		for i in range (7):
			Button = Window.GetControl (55 + i)
			Button.SetVarAssoc ("PriestSpellLevel", i)

	# Setup memorized spells buttons
	for i in range (12):
		Button = Window.GetControl (3 + i)
		color = {'r' : 0, 'g' : 0, 'b' :0, 'a' : 64}
		Button.SetBorder (0,color,0,1)
		Button.SetSprites ("SPELFRAM",0,0,0,0,0)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("SpellButton", i)
		Button.SetAnimation (None)

	# Setup book spells buttons
	for i in range (GUICommon.GetGUISpellButtonCount()):
		Button = Window.GetControl (27 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

	return

def UpdatePriestWindow (Window):
	global PriestMemorizedSpellList, PriestKnownSpellList

	PriestMemorizedSpellList = []
	PriestKnownSpellList = []

	pc = GemRB.GameGetSelectedPCSingle ()
	spelltype = IE_SPELL_TYPE_PRIEST
	level = PriestSpellLevel
	max_mem_cnt = GemRB.GetMemorizableSpellsCount (pc, spelltype, level)
	
	ClassName = GUICommon.GetClassRowName (pc)
	DivineCaster = CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL")
	if DivineCaster == "*":
		# also check the DRUIDSPELL column
		DivineCaster = CommonTables.ClassSkills.GetValue (ClassName, "DRUIDSPELL")
	CantCast = DivineCaster == "*"
	CantCast += GemRB.GetPlayerStat(pc, IE_DISABLEDBUTTON)&(1<<ACT_CAST)

	GUICommon.AdjustWindowVisibility (Window, pc, CantCast)

	Label = Window.GetControl (0x10000032)
	# bg2 uses a shorthand form
	if GameCheck.IsBG2():
		GemRB.SetToken ("SPELLLEVEL", str(level+1))
		Label.SetText (10345)
	else:
		GemRB.SetToken ('LEVEL', str(level+1))
		Label.SetText (12137)

	Name = GemRB.GetPlayerName (pc, 0)
	Label = Window.GetControl (0x10000035)
	Label.SetText (Name)

	mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
	for i in range (12):
		Button = Window.GetControl (3 + i)
		if i < mem_cnt:
			ms = GemRB.GetMemorizedSpell (pc, spelltype, level, i)
			Button.SetSpellIcon (ms['SpellResRef'], 0)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
			if ms['Flags']:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenPriestSpellUnmemorizeWindow)
			else:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnPriestUnmemorizeSpell)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenPriestSpellInfoWindow)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Button.SetTooltip (spell['SpellName'])
			PriestMemorizedSpellList.append (ms['SpellResRef'])
			Button.EnableBorder (0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				Button.SetFlags (IE_GUI_BUTTON_NORMAL | IE_GUI_BUTTON_PLAYONCE | IE_GUI_BUTTON_PLAYALWAYS, OP_SET)
			else:
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
			Button.SetTooltip ('')
			Button.EnableBorder (0, 0)

	known_cnt = GemRB.GetKnownSpellsCount (pc, spelltype, level)
	btncount = GUICommon.GetGUISpellButtonCount()
	i = 0
	for i in range (known_cnt):
		Button = Window.GetControl (27 + i)
		Button.SetAnimation (None)
		ks = GemRB.GetKnownSpell (pc, spelltype, level, i)
		Button.SetSpellIcon (ks['SpellResRef'], 0)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnPriestMemorizeSpell)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, OpenPriestSpellInfoWindow)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		Button.SetTooltip (spell['SpellName'])
		PriestKnownSpellList.append (ks['SpellResRef'])
		Button.SetVarAssoc ("SpellButton", 100 + i)

	if known_cnt == 0: i = -1
	for i in range (i + 1, btncount):
		Button = Window.GetControl (27 + i)
		Button.SetAnimation (None)
		
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		Button.SetTooltip ('')
		Button.EnableBorder (0, 0)

	Window.Focus()
	return

TogglePriestWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIPR", GUICommonWindows.ToggleWindow, InitPriestWindow, UpdatePriestWindow)
OpenPriestWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIPR", GUICommonWindows.OpenWindowOnce, InitPriestWindow, UpdatePriestWindow)

def PriestPrevLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel > 0:
		PriestSpellLevel = PriestSpellLevel - 1
		UpdatePriestWindow (PriestSpellWindow)
	return

def PriestNextLevelPress ():
	global PriestSpellLevel

	if PriestSpellLevel < 6:
		PriestSpellLevel = PriestSpellLevel + 1
		UpdatePriestWindow (PriestSpellWindow)
	return

def RefreshPriestLevel ():
	global PriestSpellLevel

	PriestSpellLevel = GemRB.GetVar ("PriestSpellLevel")
	UpdatePriestWindow (PriestSpellWindow)
	return

def OpenPriestSpellInfoWindow ():
	Window = GemRB.LoadWindow (3)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())

	index = GemRB.GetVar ("SpellButton")
	if index < 100:
		ResRef = PriestMemorizedSpellList[index]
	else:
		ResRef = PriestKnownSpellList[index - 100]

	spell = GemRB.GetSpell (ResRef)

	if GameCheck.IsBG2():
		Label = Window.GetControl (0x0fffffff)
	else:
		Label = Window.GetControl (0x10000000)
	Label.SetText (spell['SpellName'])

	Button = Window.GetControl (2)
	Button.SetSpellIcon (ResRef, 1)

	Text = Window.GetControl (3)
	Text.SetText (spell['SpellDesc'])

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnPriestMemorizeSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	if GemRB.MemorizeSpell (pc, spelltype, level, index):
		UpdatePriestWindow (PriestSpellWindow)
		GemRB.PlaySound ("GAM_24")
		Button = PriestSpellWindow.GetControl(index + 27)
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
		Button2 = PriestSpellWindow.GetControl(mem_cnt + 2)
		if GameCheck.IsBG2(): # no blending
			Button.SetAnimation ("FLASH")
			Button2.SetAnimation ("FLASH")
		else:
			Button.SetAnimation ("FLASH", 0, 1)
			Button2.SetAnimation ("FLASH", 0, 1)
	return

def OpenPriestSpellRemoveWindow ():
	Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	def RemoveSpell ():
		OnPriestRemoveSpell()
		Window.Close()

	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RemoveSpell)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenPriestSpellUnmemorizeWindow (btn, val):
	Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	def Unmemorize(btn, val):
		OnPriestUnmemorizeSpell(btn, val)
 		Window.Close()
	
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Unmemorize(btn, val))
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnPriestUnmemorizeSpell (btn, index):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	if GemRB.UnmemorizeSpell (pc, spelltype, level, index):
		UpdatePriestWindow (PriestSpellWindow)
		GemRB.PlaySound ("GAM_44")
		Button = PriestSpellWindow.GetControl(index + 3)
		Button.SetAnimation ("FLASH", 0, GameCheck.IsBG2() is False) # no blending for bg2
	return

def OnPriestRemoveSpell ():
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	index = GemRB.GetVar ("SpellButton") - 100

	#remove spell from memory
	GemRB.RemoveSpell (pc, spelltype, level, index)
	OpenPriestSpellInfoWindow()
	return

###################################################
# End of file GUIPR.py

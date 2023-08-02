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

FlashResRef = "FLASHBR" if GameCheck.IsBG2() else "FLASH"

def InitPriestWindow (Window):
	global PriestSpellWindow
	PriestSpellWindow = Window
	
	Button = Window.GetControl (1)
	Button.OnPress (PriestPrevLevelPress)

	Button = Window.GetControl (2)
	Button.OnPress (PriestNextLevelPress)

	#setup level buttons
	if GameCheck.IsBG2():
		for i in range (7):
			Button = Window.GetControl (55 + i)
			Button.OnPress (RefreshPriestLevel)
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
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetVarAssoc ("Memorized", i)
		Button.SetAnimation (None)

	# Setup book spells buttons
	for i in range (GUICommon.GetGUISpellButtonCount()):
		Button = Window.GetControl (27 + i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PLAYALWAYS, OP_OR)
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
			Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_PLAYALWAYS, OP_SET)
			if ms['Flags']:
				Button.OnPress (OpenPriestSpellUnmemorizeWindow)
			else:
				Button.OnPress (OnPriestUnmemorizeSpell)
			Button.OnRightPress (OpenPriestSpellInfoWindow)
			spell = GemRB.GetSpell (ms['SpellResRef'])
			Button.SetTooltip (spell['SpellName'])
			PriestMemorizedSpellList.append (ms['SpellResRef'])
			Button.EnableBorder (0, ms['Flags'] == 0)
		else:
			if i < max_mem_cnt:
				Button.SetFlags (IE_GUI_BUTTON_NORMAL | IE_GUI_BUTTON_PLAYALWAYS, OP_SET)
			else:
				Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PLAYALWAYS, OP_SET)
			Button.OnPress (None)
			Button.OnRightPress (None)
			Button.SetTooltip ('')
			Button.EnableBorder (0, 0)

	known_cnt = GemRB.GetKnownSpellsCount (pc, spelltype, level)
	btncount = GUICommon.GetGUISpellButtonCount()
	i = 0
	for i in range (known_cnt):
		Button = Window.GetControl (27 + i)
		ks = GemRB.GetKnownSpell (pc, spelltype, level, i)
		Button.SetSpellIcon (ks['SpellResRef'], 0)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.OnPress (OnPriestMemorizeSpell)
		Button.OnRightPress (OpenPriestSpellInfoWindow)
		spell = GemRB.GetSpell (ks['SpellResRef'])
		Button.SetTooltip (spell['SpellName'])
		PriestKnownSpellList.append (ks['SpellResRef'])
		Button.SetVarAssoc ("Memorized", i)

	if known_cnt == 0: i = -1
	for i in range (i + 1, btncount):
		Button = Window.GetControl (27 + i)
		
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		Button.OnPress (None)
		Button.OnRightPress (None)
		Button.SetTooltip ('')
		Button.EnableBorder (0, 0)

	Window.Focus()
	return

TogglePriestWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIPR", GUICommonWindows.ToggleWindow, InitPriestWindow, UpdatePriestWindow, True)
OpenPriestWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIPR", GUICommonWindows.OpenWindowOnce, InitPriestWindow, UpdatePriestWindow, True)

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

def OpenPriestSpellInfoWindow (btn):
	Window = GemRB.LoadWindow (3)

	#back
	Button = Window.GetControl (5)
	Button.SetText (15416)
	Button.OnPress (Window.Close)

	index = btn.Value
	if btn.VarName == "Memorized":
		ResRef = PriestMemorizedSpellList[index]
	else:
		ResRef = PriestKnownSpellList[index]

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

def OnPriestMemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	index = btn.Value

	if GemRB.MemorizeSpell (pc, spelltype, level, index):
		GemRB.PlaySound ("GAM_24")
		Button = PriestSpellWindow.GetControl(index + 27)
		Button.SetAnimation (FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, spelltype, level, False)
		Button2 = PriestSpellWindow.GetControl(mem_cnt + 2)
		Button2.SetAnimation (FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		UpdatePriestWindow (PriestSpellWindow)
	return

def OpenPriestSpellRemoveWindow ():
	Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	def RemoveSpell (btn):
		OnPriestRemoveSpell(btn)
		Window.Close()

	Button.OnPress (RemoveSpell)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenPriestSpellUnmemorizeWindow (btn):
	Window = GemRB.LoadWindow (5)

	# "Are you sure you want to ....?"
	TextArea = Window.GetControl (3)
	TextArea.SetText (11824)

	# Remove
	Button = Window.GetControl (0)
	Button.SetText (17507)
	def Unmemorize (btn):
		OnPriestUnmemorizeSpell (btn)
		Window.Close()
	
	Button.SetValue (btn.Value)
	Button.OnPress (Unmemorize)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OnPriestUnmemorizeSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST
	index = btn.Value

	if GemRB.UnmemorizeSpell (pc, spelltype, level, index):
		GemRB.PlaySound ("GAM_44")
		Button = PriestSpellWindow.GetControl(index + 3)
		Button.SetAnimation (FlashResRef, 0, A_ANI_PLAYONCE | A_ANI_BLEND)
		UpdatePriestWindow (PriestSpellWindow)
	return

def OnPriestRemoveSpell (btn):
	pc = GemRB.GameGetSelectedPCSingle ()
	level = PriestSpellLevel
	spelltype = IE_SPELL_TYPE_PRIEST

	index = btn.Value

	#remove spell from memory
	GemRB.RemoveSpell (pc, spelltype, level, index)
	OpenPriestSpellInfoWindow(btn)
	return

###################################################
# End of file GUIPR.py

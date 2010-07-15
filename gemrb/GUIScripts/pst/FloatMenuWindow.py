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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# FloatMenuWindow.py - display PST's floating menu window from GUIWORLD winpack

###################################################

import GemRB
import GUICommon
import CommonTables
import GUICommonWindows
from GUIDefines import *
from ie_stats import *

FloatMenuWindow = None

MENU_MODE_SINGLE = 0
MENU_MODE_GROUP = 1
MENU_MODE_WEAPONS = 2
MENU_MODE_ITEMS = 3
MENU_MODE_SPELLS = 4
MENU_MODE_ABILITIES = 5

float_menu_mode = MENU_MODE_SINGLE
float_menu_index = 0
float_menu_selected = None
type = None

def UseSpell ():
	pc = GemRB.GameGetFirstSelectedPC ()
	slot = float_menu_selected+float_menu_index
	print "spell", type, slot
	GemRB.SpellCast (pc, 1<<type, slot)
	return

def UseItem ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.UseItem (pc, float_menu_selected+20, 0)
	return

def UseWeapon ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetEquippedQuickSlot (pc, float_menu_selected)
	return

def OpenFloatMenuWindow ():
	global FloatMenuWindow
	global float_menu_mode, float_menu_index

	GemRB.HideGUI ()

	if FloatMenuWindow:
		if FloatMenuWindow:
			FloatMenuWindow.Unload ()
		FloatMenuWindow = None

		#FIXME: UnpauseGameTimer
		GemRB.GamePause (False, 0)
		GemRB.SetVar ("FloatWindow", -1)
		GUICommonWindows.SetSelectionChangeMultiHandler (None)
		GemRB.UnhideGUI ()

		if float_menu_selected==None:
			GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
			return

		if float_menu_mode == MENU_MODE_ITEMS:
			UseItem()
		elif float_menu_mode == MENU_MODE_WEAPONS:
			UseWeapon()
		elif float_menu_mode == MENU_MODE_SPELLS:
			UseSpell()
		elif float_menu_mode == MENU_MODE_ABILITIES:
			UseSpell()
		return

	ActionLevel = 0
	if not GemRB.GameGetFirstSelectedPC ():
		GemRB.UnhideGUI ()
		return
	# FIXME: remember current selection
	#FIXME: PauseGameTimer
	GemRB.GamePause (True, 0)

	GemRB.LoadWindowPack ("GUIWORLD")
	FloatMenuWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", Window.ID)

	x, y = GemRB.GetVar ("MenuX"), GemRB.GetVar ("MenuY")

	# FIXME: keep the menu inside the viewport!!!
	Window.SetPos (x, y, WINDOW_CENTER | WINDOW_BOUNDED)

	# portrait button
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectNextPC)
	Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, GUICommon.OpenFloatMenuWindow)

	# Initiate Dialogue
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectDialog)
	Button.SetTooltip (8191)

	# Attack/Select Weapon
	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectWeapons)
	Button.SetTooltip (8192)

	# Cast spell
	Button = Window.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectSpells)
	Button.SetTooltip (8193)

	# Use Item
	Button = Window.GetControl (4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectItems)
	Button.SetTooltip (8194)

	# Use Special Ability
	Button = Window.GetControl (5)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuSelectAbilities)
	Button.SetTooltip (8195)

	# Menu Anchors/Handles
	Button = Window.GetControl (11)
	Button.SetTooltip (8199)
	Button.SetFlags (IE_GUI_BUTTON_DRAGGABLE, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, FloatMenuDrag)

	Button = Window.GetControl (12)
	Button.SetTooltip (8199)
	Button.SetFlags (IE_GUI_BUTTON_DRAGGABLE, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, FloatMenuDrag)

	# Rotate Items left (to begin)
	Button = Window.GetControl (13)
	Button.SetTooltip (8197)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuPreviousItem)

	# Rotate Items right (to end)
	Button = Window.GetControl (14)
	Button.SetTooltip (8198)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, FloatMenuNextItem)

	# 6 - 10 - items/spells/other
	for i in range (6, 11):
		Button = Window.GetControl (i)
		Button.SetPos (65535, 65535)

	# 15 - 19 - empty item slot
	for i in range (15, 20):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)

	# BAMs:
	# AMALLSTP - 41655
	# AMATTCK - 41654
	#AMGENB1
	#AMGENS
	#AMGUARD - 31657, 32431, 41652, 48887
	#AMHILITE - highlight frame
	#AMINVNT - 41601, 41709
	#AMJRNL - 41623, 41714
	#AMMAP - 41625, 41710
	#AMSPLL - 4709
	#AMSTAT - 4707
	#AMTLK - 41653

	#AMPANN
	#AMPDKK
	#AMPFFG
	#AMPIGY
	#AMPMRT
	#AMPNDM
	#AMPNM1
	#AMPVHA

	num = 0
	for i in range (GemRB.GetPartySize ()):
		if GemRB.GameIsPCSelected (i + 1):
			num = num + 1

	# if more than one is selected, start in group menu mode (mode 1)
	if num == 1:
		float_menu_mode = MENU_MODE_SINGLE
	else:
		float_menu_mode = MENU_MODE_GROUP

	float_menu_index = 0

	GUICommonWindows.SetSelectionChangeMultiHandler (FloatMenuSelectAnotherPC)
	UpdateFloatMenuWindow ()

	GemRB.UnhideGUI ()
	return

def UpdateFloatMenuWindow ():
	Window = FloatMenuWindow

	pc = GemRB.GameGetFirstSelectedPC ()

	Button = Window.GetControl (0)
	Button.SetSprites (GUICommonWindows.GetActorPortrait (pc, 'FMENU'), 0, 0, 1, 2, 3)
	Button = Window.GetControl (13)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button = Window.GetControl (14)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	if float_menu_mode == MENU_MODE_SINGLE:
		for i in range (5):
			UpdateFloatMenuSingleAction (i)
	elif float_menu_mode == MENU_MODE_GROUP:
		for i in range (5):
			UpdateFloatMenuGroupAction (i)
	elif float_menu_mode == MENU_MODE_WEAPONS:
		# weapons
		for i in range (5):
			UpdateFloatMenuItem (pc, i, 1)
	elif float_menu_mode == MENU_MODE_ITEMS:
		# items
		for i in range (5):
			UpdateFloatMenuItem (pc, i, 0)
	elif float_menu_mode == MENU_MODE_SPELLS:
		# spells
		RefreshSpellList(pc, False)
		if float_menu_index:
			Button = Window.GetControl (13)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		if float_menu_index+3<len(spell_list):
			Button = Window.GetControl (14)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		for i in range (5):
			UpdateFloatMenuSpell (pc, i)
	elif float_menu_mode == MENU_MODE_ABILITIES:
		# abilities
		RefreshSpellList(pc, True)
		if float_menu_index:
			Button = Window.GetControl (13)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		if float_menu_index+3<len(spell_list):
			Button = Window.GetControl (14)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		for i in range (5):
			UpdateFloatMenuSpell (pc, i)
	return

def UpdateFloatMenuSingleAction (i):
	Window = FloatMenuWindow
	butts = [ ('AMMAP', 41625, ''),
		  ('AMJRNL', 41623, ''),
		  ('AMINVNT', 41601, ''),
		  ('AMSTAT', 4707, ''),
		  ('AMSPLL', 4709, '') ]  # or 41624
	# Map Screen
	# Journal Screen
	# Inventory screen
	# Statistics screen
	# Mage Spell Book / Priest Scroll

	Button = Window.GetControl (15 + i)

	Button.SetSprites (butts[i][0], 0, 0, 1, 2, 3)
	Button.SetTooltip (butts[i][1])
	Button.SetText ('')
	return

def UpdateFloatMenuGroupAction (i):
	Window = FloatMenuWindow
	butts = [ ('AMGUARD', 31657, ''),
		  ('AMTLK', 41653, ''),
		  ('AMATTCK', 41654, ''),
		  ('AMALLSTP', 41655, ''),
		  ('AMGENS', '', '') ]
	# Guard
	# Initiate dialogue
	# Attack
	# Abort Current Action
	Button = Window.GetControl (15 + i)

	Button.SetSprites (butts[i][0], 0, 0, 1, 2, 3)
	Button.SetTooltip (butts[i][1])
	Button.SetText ('')
	return

def RefreshSpellList(pc, innate):
	global spell_hash, spell_list, type

	if innate:
		type = IE_SPELL_TYPE_INNATE
	else:
		if (CommonTables.ClassSkillsTable.GetValue (GemRB.GetPlayerStat( pc, IE_CLASS), 1)=="*"):
			type = IE_SPELL_TYPE_WIZARD
		else:
			type = IE_SPELL_TYPE_PRIEST

	# FIXME: ugly, should be in Spellbook.cpp
	spell_hash = {}
	spell_list = []
	#level==0 is level #1
	for level in range (9):
		mem_cnt = GemRB.GetMemorizedSpellsCount (pc, type, level)
		for j in range (mem_cnt):
			ms = GemRB.GetMemorizedSpell (pc, type, level, j)

			# Spell was already used up
			if not ms['Flags']: continue

			if ms['SpellResRef'] in spell_hash:
				spell_hash[ms['SpellResRef']] = spell_hash[ms['SpellResRef']] + 1
			else:
				spell_hash[ms['SpellResRef']] = 1
				spell_list.append( ms['SpellResRef'] )
	return

def UpdateFloatMenuItem (pc, i, weapons):
	Window = FloatMenuWindow
	#no float menu index needed for weapons or quick items
	if weapons:
		slot_item = GemRB.GetSlotItem (pc, 9 + i)
	else:
		slot_item = GemRB.GetSlotItem (pc, 20 + i)
	Button = Window.GetControl (15 + i)

	#the selected state is in another bam, sucks, we have to do everything manually
	if i == float_menu_selected:
		Button.SetSprites ('AMHILITE', 0, 0, 1, 0, 0)
	else:
		Button.SetSprites ('AMGENS', 0, 0, 1, 0, 0)

	# Weapons - the last action is 'Guard'

	#if slot_item and slottype:
	if slot_item:
		item = GemRB.GetItem (slot_item['ItemResRef'])
		identified = slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED
		if not identified:
			Button.SetItemIcon ('')
			Button.SetText ('')
			Button.SetTooltip ('')
			return

		Button.SetItemIcon (slot_item['ItemResRef'])
		if item['StackAmount'] > 1:
			Button.SetText (str (slot_item['Usages0']))
		else:
			Button.SetText ('')
		if not identified or item['ItemNameIdentified'] == -1:
			Button.SetTooltip (item['ItemName'])
		else:
			Button.SetTooltip (item['ItemNameIdentified'])
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectItem)
		Button.SetVarAssoc ('ItemButton', i)
		return

	Button.SetItemIcon ('')
	Button.SetText ('')
	Button.SetTooltip ('')
	return

def SelectItem ():
	global float_menu_selected

	Window = FloatMenuWindow
	Button = GemRB.GetVar ('ItemButton')
	#simulating radiobutton+checkbox hybrid
	if float_menu_selected == Button:
		float_menu_selected = None
	else:
		float_menu_selected = Button
	UpdateFloatMenuWindow()
	return

def UpdateFloatMenuSpell (pc, i):
	Window = FloatMenuWindow

	Button = Window.GetControl (15 + i)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	if i == float_menu_selected:
		Button.SetBAM ('AMHILITE', 0, 0)
	else:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)

	if i + float_menu_index < len (spell_list):
		SpellResRef = spell_list[i + float_menu_index]
		Button.SetSpellIcon (SpellResRef)
		Button.SetText ("%d" %spell_hash[SpellResRef])

		spell = GemRB.GetSpell (SpellResRef)
		Button.SetTooltip (spell['SpellName'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectItem)
		Button.SetVarAssoc ('ItemButton', i)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetSpellIcon ('')
		Button.SetText ('')
		Button.SetTooltip ('')
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	return

def FloatMenuSelectNextPC ():
	sel = GemRB.GameGetFirstSelectedPC ()
	if sel == 0:
		GUICommon.OpenFloatMenuWindow ()
		return

	GemRB.GameSelectPC (sel % GemRB.GetPartySize () + 1, 1, SELECT_REPLACE)
	# NOTE: it invokes FloatMenuSelectAnotherPC() through selection change handler
	return

def FloatMenuSelectAnotherPC ():
	global float_menu_mode, float_menu_index, float_menu_selected
	float_menu_mode = MENU_MODE_SINGLE
	float_menu_index = 0
	float_menu_selected = None
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectDialog ():
	global float_menu_selected
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK)
	float_menu_selected = None
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectWeapons ():
	global float_menu_mode, float_menu_index, float_menu_selected
	float_menu_mode = MENU_MODE_WEAPONS
	float_menu_index = 0
	float_menu_selected = None
	# FIXME: Force attack mode
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectItems ():
	global float_menu_mode, float_menu_index, float_menu_selected
	float_menu_mode = MENU_MODE_ITEMS
	float_menu_index = 0
	float_menu_selected = None
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectSpells ():
	global float_menu_mode, float_menu_index, float_menu_selected
	float_menu_mode = MENU_MODE_SPELLS
	float_menu_index = 0
	float_menu_selected = None
	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectAbilities ():
	global float_menu_mode, float_menu_index, float_menu_selected
	float_menu_mode = MENU_MODE_ABILITIES
	float_menu_index = 0
	float_menu_selected = None
	UpdateFloatMenuWindow ()
	return

def FloatMenuPreviousItem ():
	global float_menu_index, float_menu_selected
	if float_menu_index > 0:
		float_menu_index = float_menu_index - 1
		if float_menu_selected!=None:
			float_menu_selected = float_menu_selected + 1
		UpdateFloatMenuWindow ()
	return

def FloatMenuNextItem ():
	global float_menu_index, float_menu_selected
	float_menu_index = float_menu_index + 1
	if float_menu_selected!=None:
		float_menu_selected = float_menu_selected - 1
	UpdateFloatMenuWindow ()
	return

def FloatMenuDrag ():
	dx, dy = GemRB.GetVar ("DragX"), GemRB.GetVar ("DragY")
	FloatMenuWindow.SetPos (dx, dy, WINDOW_RELATIVE | WINDOW_BOUNDED)
	return

#############################

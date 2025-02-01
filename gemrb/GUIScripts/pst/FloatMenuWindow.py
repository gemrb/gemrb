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
import ActionsWindow
import GUICommon
import CommonTables
import Spellbook
import GUICommonWindows
from GUIDefines import *
from ie_stats import *
from ie_action import *

import GUIMA
import GUIJRNL
import GUIINV
import GUIREC
import GUIMG
import GUIPR

FloatMenuWindow = None

CID_PORTRAIT = 0
CID_DIALOG = 1
CID_WEAPONS = 2
CID_SPELLS = 3
CID_ITEMS = 4
CID_ABILITIES = 5
# buttons 6-10 are completely redundant
CID_HANDLE1 = 11
CID_HANDLE2 = 12
CID_PREV = 13
CID_NEXT = 14
CID_SLOTS = 15 # up to 19

SLOT_COUNT = 5


MENU_MODE_SINGLE = 0
MENU_MODE_GROUP = 1
MENU_MODE_WEAPONS = 2
MENU_MODE_ITEMS = 3
MENU_MODE_SPELLS = 4
MENU_MODE_ABILITIES = 5
MENU_MODE_DIALOG = 6

float_menu_mode = MENU_MODE_SINGLE
float_menu_index = 0
float_menu_selected = None
spelltype = None

def UseSpell ():
	pc = GemRB.GameGetFirstSelectedPC ()
	slot = float_menu_selected+float_menu_index
	print("spell", spelltype, slot)
	GemRB.SpellCast (pc, 1<<type, slot)
	return

def UseItem ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.UseItem (pc, float_menu_selected+21, 0)
	return

def UseWeapon ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetEquippedQuickSlot (pc, float_menu_selected)
	return

def DoSingleAction (btn):
	i = btn.Value
	OpenFloatMenuWindow ()
	if i == 0:
		GUIMA.OpenMapWindow ()
	elif i == 1:
		GUIJRNL.OpenJournalWindow ()
	elif i == 2:
		GUIINV.OpenInventoryWindow ()
	elif i == 3:
		GUIREC.OpenRecordsWindow ()
	elif i == 4:
		pc = GemRB.GameGetFirstSelectedPC ()
		ClassName = GUICommon.GetClassRowName (pc)
		if CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL") == "*":
			GUIMG.OpenSpellWindow ()
		else:
			GUIPR.OpenPriestWindow ()

def OpenFloatMenuWindow (x=0, y=0):
	global FloatMenuWindow
	global float_menu_mode, float_menu_index, float_menu_selected

	handler = GUICommonWindows.SelectionChangeHandler
	def OnClose():
		GemRB.GamePause (False, 0)
		GUICommonWindows.SetSelectionChangeHandler (handler)

		if float_menu_selected==None:
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

	if not GemRB.GameGetFirstSelectedPC ():
		return

	GemRB.GamePause (True, 0)
	GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

	FloatMenuWindow = Window = GemRB.LoadWindow (3, "GUIWORLD")
	Window.OnUnFocus(lambda: GemRB.SetTimer(Window.Close, 0, 0))

	Window.SetFlags (WF_DRAGGABLE|WF_ALPHA_CHANNEL, OP_OR)
	Window.SetPos (x, y)
	Window.SetTooltip (8199)
	Window.OnClose (OnClose)

	# portrait button
	Button = Window.GetControl (CID_PORTRAIT)
	Button.OnPress (FloatMenuSelectNextPC)
	Button.OnRightPress (Window.Close)

	# Initiate Dialogue
	Button = Window.GetControl (CID_DIALOG)
	Button.OnPress (FloatMenuSelectDialog)
	Button.SetTooltip (8191)

	# Attack/Select Weapon
	Button = Window.GetControl (CID_WEAPONS)
	Button.OnPress (FloatMenuSelectWeapons)
	Button.SetTooltip (8192)

	# Cast spell
	Button = Window.GetControl (CID_SPELLS)
	Button.OnPress (FloatMenuSelectSpells)
	Button.SetTooltip (8193)

	# Use Item
	Button = Window.GetControl (CID_ITEMS)
	Button.OnPress (FloatMenuSelectItems)
	Button.SetTooltip (8194)

	# Use Special Ability
	Button = Window.GetControl (CID_ABILITIES)
	Button.OnPress (FloatMenuSelectAbilities)
	Button.SetTooltip (8195)

	# Menu Anchors/Handles
	Handle = Window.GetControl (CID_HANDLE1)
	Handle.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	Handle = Window.GetControl (CID_HANDLE2)
	Handle.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	
	# Rotate Items left (to begin)
	Button = Window.GetControl (CID_PREV)
	Button.SetTooltip (8197)
	Button.OnPress (FloatMenuPreviousItem)

	# Rotate Items right (to end)
	Button = Window.GetControl (CID_NEXT)
	Button.SetTooltip (8198)
	Button.OnPress (FloatMenuNextItem)

	# 6 - 10 - items/spells/other
	for i in range (CID_ABILITIES + 1, CID_HANDLE1):
		if Window.GetControl (i):
			Window.DeleteControl (i)

	# 15 - 19 - empty item slot
	for i in range (SLOT_COUNT):
		Button = Window.GetControl (CID_SLOTS + i)
		Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)
		Button.SetVarAssoc ("ItemButton", i)
		Button.SetFont ('NUMBER')
		Button.SetPushOffset (0, 0)

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
		float_menu_selected = 4 # "Abort current action" slot

	float_menu_index = 0

	def SelChange():
		FloatMenuSelectAnotherPC()
		if handler:
			handler()

	GUICommonWindows.SetSelectionChangeHandler (SelChange)
	UpdateFloatMenuWindow ()

	return

def UpdateFloatMenuWindow ():
	Window = FloatMenuWindow
	# this enables the use of hotkeys without the window being open
	if not FloatMenuWindow:
		return

	pc = GemRB.GameGetFirstSelectedPC ()

	Button = Window.GetControl (CID_PORTRAIT)
	if not Button:
		return
	Button.SetSprites (GUICommonWindows.GetActorPortrait (pc, 'FMENU'), 0, 0, 1, 2, 3)
	Button = Window.GetControl (CID_PREV)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button = Window.GetControl (CID_NEXT)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	updaterFunc = None
	if float_menu_mode == MENU_MODE_SINGLE:
		updaterFunc = UpdateFloatMenuSingleAction
	elif float_menu_mode == MENU_MODE_GROUP:
		updaterFunc = UpdateFloatMenuGroupAction
	elif float_menu_mode == MENU_MODE_WEAPONS:
		# weapons
		updaterFunc = lambda i: UpdateFloatMenuItem (pc, i, 1)
	elif float_menu_mode == MENU_MODE_ITEMS:
		# items
		updaterFunc = lambda i: UpdateFloatMenuItem (pc, i, 0)
	elif float_menu_mode == MENU_MODE_SPELLS:
		# spells
		RefreshSpellList(pc, False)
		Button = Window.GetControl (CID_PREV)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button = Window.GetControl (CID_NEXT)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		updaterFunc = lambda i: UpdateFloatMenuSpell (pc, i)
	elif float_menu_mode == MENU_MODE_ABILITIES:
		# abilities
		RefreshSpellList(pc, True)
		if float_menu_index:
			Button = Window.GetControl (CID_PREV)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		if float_menu_index+3<len(spell_list):
			Button = Window.GetControl (CID_NEXT)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
		updaterFunc = lambda i: UpdateFloatMenuSpell (pc, i)
	elif float_menu_mode == MENU_MODE_DIALOG:
		updaterFunc = ClearSlot

	if not updaterFunc:
		return
	for i in range (SLOT_COUNT):
		updaterFunc (i)

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

	Button = Window.GetControl (CID_SLOTS + i)

	Button.SetSprites (butts[i][0], 0, 0, 1, 2, 3)
	Button.SetPicture(None)
	Button.SetTooltip (butts[i][1])
	Button.SetText ('')
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.OnPress (DoSingleAction)


def UpdateFloatMenuGroupAction (i):
	Window = FloatMenuWindow

	butts = [ ACT_DEFEND, ACT_TALK, ACT_ATTACK, ACT_STOP, ACT_SEARCH ]
	# Guard
	# Initiate dialogue
	# Attack
	# Abort Current Action
	Button = Window.GetControl (CID_SLOTS + i)

	Button.SetActionIcon (globals(), butts[i], i+1 )
	Button.SetState (IE_GUI_BUTTON_ENABLED)

def RefreshSpellList(pc, innate):
	global spell_list, type

	if innate:
		spelltype = IE_SPELL_TYPE_INNATE
	else:
		ClassName = GUICommon.GetClassRowName (pc)
		if CommonTables.ClassSkills.GetValue (ClassName, "CLERICSPELL") == "*":
			spelltype = IE_SPELL_TYPE_WIZARD
		else:
			spelltype = IE_SPELL_TYPE_PRIEST

	spelltype = 1 << spelltype
	spell_list = []
	for i in range(16):
		if spelltype & (1<<i):
			spell_list += Spellbook.GetUsableMemorizedSpells (pc, i)

	GemRB.SetVar ("Type", spelltype)
	GemRB.SetVar ("QSpell", -1)
	return

def UpdateFloatMenuItem (pc, i, weapons):
	Window = FloatMenuWindow
	#no float menu index needed for weapons or quick items
	if weapons:
		slot_item = GemRB.GetSlotItem (pc, 10 + i)
		if i == 0 and not slot_item:
				# default weapon
				slot_item = GemRB.GetSlotItem (pc, 10 + i, 1)
	else:
		slot_item = GemRB.GetSlotItem (pc, 21 + i)
	Button = Window.GetControl (CID_SLOTS + i)

	#the selected state is in another bam, sucks, we have to do everything manually
	if i == float_menu_selected:
		Button.SetSprites ('AMHILITE', 0, 0, 1, 0, 0)
	else:
		Button.SetSprites ('AMGENS', 0, 0, 1, 0, 0)

	# Weapons - the last action is 'Guard'
	if weapons and i==4:
		# FIXME: rather call UpdateFloatMenuGroupAction?
		Button.SetActionIcon (globals(), ACT_DEFEND)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		return
		
	#if slot_item and slottype:
	if slot_item and slot_item['Flags'] & IE_INV_ITEM_IDENTIFIED:
		item = GemRB.GetItem (slot_item['ItemResRef'])

		Button.SetItemIcon (slot_item['ItemResRef'], 6)
		if item['MaxStackAmount'] > 1:
			Button.SetText (str (slot_item['Usages0']))
		else:
			Button.SetText ('')
		if item['ItemNameIdentified'] == -1:
			Button.SetTooltip (item['ItemName'])
		else:
			Button.SetTooltip (item['ItemNameIdentified'])
		#Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
		Button.OnPress (SelectItem)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		return

	ClearSlot (i)


def SelectItem (btn):
	global float_menu_selected

	#simulating radiobutton+checkbox hybrid
	if float_menu_selected == btn.Value:
		float_menu_selected = None
	else:
		float_menu_selected = btn.Value
	if GUICommon.UsingTouchInput ():
		FloatMenuWindow.Close ()
	else:
		UpdateFloatMenuWindow()
	return

def UpdateFloatMenuSpell (pc, i):
	Window = FloatMenuWindow

	Button = Window.GetControl (15 + i)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	if i == float_menu_selected:
		Button.SetBAM ('AMHILITE', 0, 0)
	else:
		Button.SetPicture(None)
		#Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)

	def CloseOnPress (callback):
		if GUICommon.UsingTouchInput ():
			FloatMenuWindow.Close ()
		callback ()

	if i + float_menu_index < len (spell_list):
		SpellResRef = spell_list[i + float_menu_index]["SpellResRef"]
		Button.SetSpellIcon (SpellResRef)
		Button.SetText ("%d" % spell_list[i + float_menu_index]["MemoCount"])

		spell = GemRB.GetSpell (SpellResRef)
		Button.SetTooltip (spell['SpellName'])
		Button.OnPress (lambda: CloseOnPress (ActionsWindow.SpellPressed))
		Button.SetVarAssoc ("Spell", spell_list[i + float_menu_index]['SpellIndex'])
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	elif i < 3 and GemRB.GetPlayerStat (pc, IE_CLASS) == 9:
		# handle thieving
		thieving = [ ACT_SEARCH, ACT_THIEVING, ACT_STEALTH ]
		acts = [ ActionsWindow.ActionSearchPressed, ActionsWindow.ActionThievingPressed, ActionsWindow.ActionStealthPressed ]
		Button.SetActionIcon (globals(), thieving[i])
		Button.OnPress (lambda: CloseOnPress (acts[i]))
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		ClearSlot (i)

def ClearSlot (i):
	Button = FloatMenuWindow.GetControl (CID_SLOTS + i)
	Button.SetSprites ('AMGENS', 0, 0, 1, 0, 0)
	Button.SetPicture (None)
	Button.SetText ('')
	Button.SetState (IE_GUI_BUTTON_DISABLED)

def FloatMenuSelectPreviousPC ():
	sel = GemRB.GameGetFirstSelectedPC ()
	if sel == 0:
		OpenFloatMenuWindow ()
		return

	previous = sel % GemRB.GetPartySize () - 1
	if previous == -1:
		previous = 1
	elif previous == 0:
		previous = GemRB.GetPartySize ()
	GemRB.GameSelectPC (previous, 1, SELECT_REPLACE)
	# NOTE: it invokes FloatMenuSelectAnotherPC() through selection change handler
	return

def FloatMenuSelectNextPC ():
	sel = GemRB.GameGetFirstSelectedPC ()
	if sel == 0:
		OpenFloatMenuWindow ()
		return

	GemRB.GameSelectPC (sel % GemRB.GetPartySize () + 1, 1, SELECT_REPLACE)
	# NOTE: it invokes FloatMenuSelectAnotherPC() through selection change handler
	return

def ResetMenuState (mode):
	global float_menu_mode, float_menu_index, float_menu_selected

	float_menu_mode = mode
	float_menu_index = 0
	float_menu_selected = None

def FloatMenuSelectAnotherPC ():
	ResetMenuState (MENU_MODE_SINGLE)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectDialog ():
	ResetMenuState (MENU_MODE_DIALOG)
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK)
	# close it immediately for touch users, since it's hard for them rightclick to close
	if GUICommon.UsingTouchInput ():
		FloatMenuWindow.Close ()
	else:
		UpdateFloatMenuWindow ()
	return

def FloatMenuSelectWeapons ():
	ResetMenuState (MENU_MODE_WEAPONS)
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectItems ():
	ResetMenuState (MENU_MODE_ITEMS)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectSpells ():
	pc = GemRB.GameGetFirstSelectedPC ()
	ClassName = GUICommon.GetClassRowName (pc)
	BookType = CommonTables.ClassSkills.GetValue (ClassName, "BOOKTYPE", GTV_INT)
	if not BookType:
		return

	ResetMenuState (MENU_MODE_SPELLS)
	UpdateFloatMenuWindow ()
	return

def FloatMenuSelectAbilities ():
	ResetMenuState (MENU_MODE_ABILITIES)
	UpdateFloatMenuWindow ()
	return

def FloatMenuPreviousItem ():
	global float_menu_index, float_menu_selected
	if float_menu_mode == MENU_MODE_SPELLS and float_menu_index <= 0:
		# enable backcycling for spells
		float_menu_index = len(spell_list) - SLOT_COUNT + 1
	if float_menu_index > 0:
		float_menu_index = float_menu_index - 1
		if float_menu_selected!=None:
			float_menu_selected = float_menu_selected + 1
		UpdateFloatMenuWindow ()
	return

def FloatMenuNextItem ():
	global float_menu_index, float_menu_selected
	if float_menu_mode == MENU_MODE_SPELLS and float_menu_index >= len(spell_list) - SLOT_COUNT:
		# enable overflow for spells
		float_menu_index = 0
	else:
		float_menu_index = float_menu_index + 1
	if float_menu_selected!=None:
		float_menu_selected = float_menu_selected - 1
	UpdateFloatMenuWindow ()
	return

#############################

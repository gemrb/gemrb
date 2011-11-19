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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUICommonWindows.py - functions to open common
# windows in lower part of the screen
###################################################

import GemRB
from GUIDefines import *
from ie_stats import IE_STATE_ID, STATE_DEAD
from ie_modal import *
from ie_action import *
import GUICommon
import CommonTables
import LUCommon
import InventoryCommon
import Spellbook

# needed for all the Open*Window callbacks in the OptionsWindow
import GUIJRNL
import GUIMA
import GUIMG
import GUIINV
import GUIOPT
import GUIPR
import GUIREC

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

PortraitWindow = None
OptionsWindow = None
ActionsWindow = None
DraggedPortrait = None

def SetupMenuWindowControls (Window, Gears, ReturnToGame):
	"""Sets up all of the basic control windows."""

	global OptionsWindow

	OptionsWindow = Window
	# FIXME: add "(key)" to tooltips!

	# Return to Game
	Button = Window.GetControl (0)
	Button.SetTooltip (16313)
	Button.SetVarAssoc ("SelectedWindow", 0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReturnToGame)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	if GUICommon.GameIsBG1():
		# enabled BAM isn't present in .chu, defining it here
		Button.SetSprites ("GUILSOP", 0,16,17,28,16)

	# Map
	Button = Window.GetControl (1)
	Button.SetTooltip (16310)
	Button.SetVarAssoc ("SelectedWindow", 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMA.OpenMapWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,0,1,20,0)

	# Journal
	Button = Window.GetControl (2)
	Button.SetTooltip (16308)
	Button.SetVarAssoc ("SelectedWindow", 2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIJRNL.OpenJournalWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,4,5,22,4)

	# Inventory
	Button = Window.GetControl (3)
	Button.SetTooltip (16307)
	Button.SetVarAssoc ("SelectedWindow", 3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.OpenInventoryWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,2,3,21,2)

	# Records
	Button = Window.GetControl (4)
	Button.SetTooltip (16306)
	Button.SetVarAssoc ("SelectedWindow", 4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIREC.OpenRecordsWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,6,7,23,6)

	# Mage
	Button = Window.GetControl (5)
	Button.SetTooltip (16309)
	Button.SetVarAssoc ("SelectedWindow", 5)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMG.OpenMageWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,8,9,24,8)

	# Priest
	Button = Window.GetControl (6)
	Button.SetTooltip (14930)
	Button.SetVarAssoc ("SelectedWindow", 6)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIPR.OpenPriestWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,10,11,25,10)

	# Options
	Button = Window.GetControl (7)
	Button.SetTooltip (16311)
	Button.SetVarAssoc ("SelectedWindow", 7)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIOPT.OpenOptionsWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,12,13,26,12)

	# Party mgmt
	Button = Window.GetControl (8)
	Button.SetTooltip (16312)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: OpenPartyWindow

	# pause button
	if Gears:
		# Pendulum, gears, sun/moon dial (time)
		# FIXME: display all animations: CPEN, CGEAR, CDIAL
		if GUICommon.HasHOW(): # how doesn't have this in the right place
			pos = GemRB.GetSystemVariable (SV_HEIGHT)-71
			Window.CreateButton (9, 6, pos, 64, 71)
		Button = Window.GetControl (9)
		if GUICommon.GameIsBG2():
			Label = Button.CreateLabelOnButton (0x10000009, "NORMAL", 0)
			Label.SetAnimation ("CPEN")

		Button.SetAnimation ("CGEAR")
		if GUICommon.GameIsBG2():
			Button.SetBAM ("CDIAL", 0, 0)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
		GUICommon.SetGamedaysAndHourToken()
		Button.SetTooltip(16041)
		rb = 11
	else:
		rb = 9

	# Rest
	if Window.HasControl (rb):
		Button = Window.GetControl (rb)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)
		Button.SetTooltip (11942)

	MarkMenuButton (Window)

	if PortraitWindow:
		UpdatePortraitWindow ()
	return

def MarkMenuButton (WindowIndex):
	Pressed = WindowIndex.GetControl( GemRB.GetVar ("SelectedWindow") )

	for button in range (9):
		Button = WindowIndex.GetControl (button)
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	if Pressed:
		Button = Pressed
	else: # highlight return to game
		Button = WindowIndex.GetControl (0)
	Button.SetState (IE_GUI_BUTTON_SELECTED)

	return

def OptionsPress ():
	"""Toggles the options pane """
	PP = GemRB.GetMessageWindowSize () & GS_OPTIONPANE
	if PP:
		GemRB.GameSetScreenFlags (GS_OPTIONPANE, OP_NAND)
	else:
		GemRB.GameSetScreenFlags (GS_OPTIONPANE, OP_OR)
	return

def PortraitPress ():
	"""Toggles the portraits pane """
	PP = GemRB.GetMessageWindowSize () & GS_PORTRAITPANE
	if PP:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_NAND)
	else:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_OR)
	return

def AIPress ():
	"""Toggles the party AI."""

	Button = PortraitWindow.GetControl (6)
	AI = GemRB.GetMessageWindowSize () & GS_PARTYAI

	if AI:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_NAND)
		GemRB.SetVar ("AI", 0)
		Button.SetTooltip (15918)
	else:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
		GemRB.SetVar ("AI", GS_PARTYAI)
		Button.SetTooltip (15917)

	#force redrawing, in case a hotkey triggered this function
	Button.SetVarAssoc ("AI", GS_PARTYAI)
	return

def EmptyControls ():
	global ActionsWindow

	Selected = GemRB.GetSelectedSize()
	if Selected==1:
		pc = GemRB.GameGetFirstSelectedActor ()
		#init spell list
		GemRB.SpellCast (pc, -1, 0)

	GemRB.SetVar ("ActionLevel", 0)
	Window = ActionsWindow
	for i in range (12):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetPicture ("")
		Button.SetText ("")
		Button.SetActionIcon (globals(), -1)
	return

def SelectFormationPreset ():
	"""Choose the default formation."""
	GemRB.GameSetFormation (GemRB.GetVar ("Value"), GemRB.GetVar ("Formation") )
	GroupControls ()
	return

def SetupFormation ():
	"""Opens the formation selection section."""
	global ActionsWindow

	Window = ActionsWindow
	for i in range (12):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%i,0,0,-1)
		Button.SetVarAssoc ("Value", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectFormationPreset)
	return

def GroupControls ():
	"""Sections that control group actions."""

	global ActionsWindow

	GemRB.SetVar ("ActionLevel", 0)
	Window = ActionsWindow
	Button = Window.GetControl (0)
	if GUICommon.GameIsBG2(): # no guard icon
		Button.SetActionIcon (globals(), -1)
	else:
		Button.SetActionIcon (globals(), 14)
	Button = Window.GetControl (1)
	Button.SetActionIcon (globals(), 7)
	Button = Window.GetControl (2)
	Button.SetActionIcon (globals(), 15)
	Button = Window.GetControl (3)
	Button.SetActionIcon (globals(), 21)
	Button = Window.GetControl (4)
	Button.SetActionIcon (globals(), -1)
	Button = Window.GetControl (5)
	Button.SetActionIcon (globals(), -1)
	Button = Window.GetControl (6)
	Button.SetActionIcon (globals(), -1)
	GemRB.SetVar ("Formation", GemRB.GameGetFormation ())
	for i in range (5):
		Button = Window.GetControl (7+i)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		idx = GemRB.GameGetFormation (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
		# kill the previous sprites or they show through
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%idx,0,0,-1)
		Button.SetVarAssoc ("Formation", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectFormation)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, SetupFormation)
		Button.SetTooltip (4935, 8+i)
	return

def OpenActionsWindowControls (Window):
	global ActionsWindow

	ActionsWindow = Window
	# 1280 and higher don't have this control
	if not Window.HasControl (62):
		UpdateActionsWindow ()
		return
	# Gears (time) when options pane is down
	Button = Window.GetControl (62)
	Label = Button.CreateLabelOnButton (0x1000003e, "NORMAL", 0)

	# FIXME: display all animations
	Label.SetAnimation ("CPEN")
	Button.SetAnimation ("CGEAR")
	Button.SetBAM ("CDIAL", 0, 0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
	GUICommon.SetGamedaysAndHourToken()
	Button.SetTooltip(16041)
	UpdateActionsWindow ()
	return

def SelectItemAbility():
	pc = GemRB.GameGetFirstSelectedActor ()
	slot = GemRB.GetVar ("Slot")
	ability = GemRB.GetVar ("Ability")
	GemRB.SetupQuickSlot (pc, 0, slot, ability)
	GemRB.SetVar ("ActionLevel", 0)
	return

def SetupItemAbilities(pc, slot):
	Window = ActionsWindow

	slot_item = GemRB.GetSlotItem(pc, slot)
	if not slot_item:
		# empty quickslot
		return
	item = GemRB.GetItem (slot_item["ItemResRef"])
	Tips = item["Tooltips"]

	for i in range (12):
		Button = Window.GetControl (i)
		Button.SetPicture ("")
		if i<len(Tips):
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
			Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
			Button.SetItemIcon (slot_item['ItemResRef'], i+6)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectItemAbility)
			Button.SetVarAssoc ("Ability", i)

			Button.SetTooltip ("F%d - %s"%(i+1,GemRB.GetString(Tips[i])) )
		else:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	return

def UpdateActionsWindow ():
	"""Redraws the actions section of the window."""

	global ActionsWindow, PortraitWindow, OptionsWindow
	global level, TopIndex

	if ActionsWindow == -1:
		return

	if ActionsWindow == None:
		return

	#fully redraw the side panes to cover the actions window
	#do this only when there is no 'otherwindow'
	if GemRB.GetVar ("OtherWindow") == -1:
		if PortraitWindow:
			PortraitWindow.Invalidate ()
		if OptionsWindow:
			OptionsWindow.Invalidate ()

	Selected = GemRB.GetSelectedSize()

	#setting up the disabled button overlay (using the second border slot)
	for i in range (12):
		Button = ActionsWindow.GetControl (i)
		if GUICommon.GameIsBG1():
			Button.SetBorder (0,6,6,4,4,0,254,0,255)
		Button.SetBorder (1, 0, 0, 0, 0, 50,30,10,120, 0, 1)
		Button.SetFont ("NUMBER")
		Button.SetText ("")

	if Selected == 0:
		EmptyControls ()
		return
	if Selected > 1:
		GroupControls ()
		return

	#we are sure there is only one actor selected
	pc = GemRB.GameGetFirstSelectedActor ()

	level = GemRB.GetVar ("ActionLevel")
	TopIndex = GemRB.GetVar ("TopIndex")
	if level == 0:
		#this is based on class
		ActionsWindow.SetupControls (globals(), pc, 0)
	elif level == 1:
		ActionsWindow.SetupEquipmentIcons(globals(), pc, TopIndex, 0)
	elif level == 2: #spells
		GemRB.SetVar ("Type", 3)
		Spellbook.SetupSpellIcons(ActionsWindow, 3, TopIndex, 0)
	elif level == 3: #innates
		GemRB.SetVar ("Type", 4)
		Spellbook.SetupSpellIcons(ActionsWindow, 4, TopIndex, 0)
	elif level == 4: #quick weapon/item ability selection
		SetupItemAbilities(pc, GemRB.GetVar("Slot") )
	elif level == 5: #all known mage spells
		GemRB.SetVar ("Type", -1)
		Spellbook.SetupSpellIcons(ActionsWindow, -1, TopIndex, 0)
	return

def ActionQWeaponPressed (which):
	"""Selects the given quickslot weapon if possible."""

	pc = GemRB.GameGetFirstSelectedActor ()
	qs = GemRB.GetEquippedQuickSlot (pc, 1)

	#38 is the magic slot
	if ((qs==which) or (qs==38)) and GemRB.GameControlGetTargetMode() != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which, -1)

	ActionsWindow.SetupControls (globals(), pc, 0)
	UpdateActionsWindow ()
	return

def ActionQWeapon1Pressed ():
	ActionQWeaponPressed(0)

def ActionQWeapon2Pressed ():
	ActionQWeaponPressed(1)

def ActionQWeapon3Pressed ():
	ActionQWeaponPressed(2)

def ActionQWeapon4Pressed ():
	ActionQWeaponPressed(3)

def ActionQSpellPressed (which):
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.SpellCast (pc, -2, which)
	UpdateActionsWindow ()
	return

def ActionQSpell1Pressed ():
	ActionQSpellPressed(0)

def ActionQSpell2Pressed ():
	ActionQSpellPressed(1)

def ActionQSpell3Pressed ():
	ActionQSpellPressed(2)

def ActionQSpellRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 2)
	UpdateActionsWindow ()
	return

def ActionQSpell1RightPressed ():
	ActionQSpellRightPressed(0)

def ActionQSpell2RightPressed ():
	ActionQSpellRightPressed(1)

def ActionQSpell3RightPressed ():
	ActionQSpellRightPressed(2)

# can't pass the globals dictionary from another module
def SetActionIconWorkaround(Button, action, function):
	Button.SetActionIcon (globals(), action, function)

#no check needed because the button wouldn't be drawn if illegal
def ActionLeftPressed ():
	"""Scrolls the actions window left.

	Used primarily for spell selection."""

	TopIndex = GemRB.GetVar ("TopIndex")
	if TopIndex>10:
		TopIndex -= 10
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

#no check needed because the button wouldn't be drawn if illegal
def ActionRightPressed ():
	"""Scrolls the action window right.

	Used primarily for spell selection."""

	pc = GemRB.GameGetFirstSelectedActor ()
	TopIndex = GemRB.GetVar ("TopIndex")
	Type = GemRB.GetVar ("Type")
	#Type is a bitfield if there is no level given
	#This is to make sure cleric/mages get all spells listed
	if Type&128 or GemRB.GetVar ("ActionLevel") == 5:
		#Max = GemRB.GetKnownSpellsCount(pc, Type&127, -1)
		tmpType = Type&127
		if tmpType == 3:
			Max = len(Spellbook.GetKnownSpells (pc, 0) + Spellbook.GetKnownSpells (pc, 1))
		else:
			Max = len(Spellbook.GetKnownSpells (pc, tmpType))
	else:
		Max = GemRB.GetMemorizedSpellsCount(pc, Type, -1, 1)
	TopIndex += 10
	if TopIndex > Max - 10:
		if Max>10:
			TopIndex = Max-10
		else:
			TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

def ActionMeleePressed ():
	""" switches to the most damaging melee weapon"""
	#get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipMostDamagingMelee()", pc)
	return

def ActionRangePressed ():
	""" switches to the most damaging ranged weapon"""
	#get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipRanged()", pc)
	return

def ActionBardSongPressed ():
	"""Toggles the battle song."""

	##FIXME: check if the actor can actually switch to this state
	#get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_BATTLESONG)
	GemRB.PlaySound ("act_01")
	UpdateActionsWindow ()
	return

def ActionSearchPressed ():
	"""Toggles detect traps."""

	##FIXME: check if the actor can actually switch to this state
	#get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DETECTTRAPS)
	UpdateActionsWindow ()
	return

def ActionStealthPressed ():
	"""Toggles stealth."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_STEALTH)
	GemRB.PlaySound ("act_07")
	UpdateActionsWindow ()
	return

def ActionTurnPressed ():
	"""Toggles turn undead."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_TURNUNDEAD)
	GemRB.PlaySound ("act_06")
	UpdateActionsWindow ()
	return

def ActionUseItemPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 1)
	UpdateActionsWindow ()
	return

def ActionCastPressed ():
	"""Opens the spell choice scrollbar."""
	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 2)
	UpdateActionsWindow ()
	return

def ActionQItemPressed (action):
	"""Uses the given quick item."""
	pc = GemRB.GameGetFirstSelectedActor ()
	#quick slot
	GemRB.UseItem (pc, -2, action, -1)
	return

def ActionQItem1Pressed ():
	ActionQItemPressed (ACT_QSLOT1)
	return

def ActionQItem2Pressed ():
	ActionQItemPressed (ACT_QSLOT2)
	return

def ActionQItem3Pressed ():
	ActionQItemPressed (ACT_QSLOT3)
	return

def ActionQItem4Pressed ():
	ActionQItemPressed (ACT_QSLOT4)
	return

def ActionQItem5Pressed ():
	ActionQItemPressed (ACT_QSLOT5)
	return

def ActionQItemRightPressed (action):
	"""Selects the used ability of the quick item."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetVar ("Slot", action)
	GemRB.SetVar ("ActionLevel", 4)
	UpdateActionsWindow ()
	return

def ActionQItem1RightPressed ():
	ActionQItemRightPressed (19)

def ActionQItem2RightPressed ():
	ActionQItemRightPressed (20)

def ActionQItem3RightPressed ():
	ActionQItemRightPressed (21)

def ActionQItem4RightPressed ():
	ActionQItemRightPressed (22)

def ActionQItem5RightPressed ():
	ActionQItemRightPressed (23)

def ActionQWeapon1RightPressed ():
	ActionQItemRightPressed (10)

def ActionQWeapon2RightPressed ():
	ActionQItemRightPressed (11)

def ActionQWeapon3RightPressed ():
	ActionQItemRightPressed (12)

def ActionQWeapon4RightPressed ():
	ActionQItemRightPressed (13)

def ActionInnatePressed ():
	"""Opens the innate spell scrollbar."""
	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 3)
	UpdateActionsWindow ()
	return

def SpellPressed ():
	"""Prepares a spell to be cast."""

	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Spell = GemRB.GetVar ("Spell")
	Type = GemRB.GetVar ("Type")
	slot = GemRB.GetVar ("QSpell")
	if slot>=0:
		#setup quickspell slot
		#if spell has no target, return
		#otherwise continue with casting
		Target = GemRB.SetupQuickSpell (pc, slot, Spell, Type)
		#sabotage the immediate casting of self targeting spells
		if Target == 5 or Target == 7:
			Type = -1
			GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

	if Type==-1:
		GemRB.SetVar ("ActionLevel", 0)
		GemRB.SetVar("Type", 0)
	GemRB.SpellCast (pc, Type, Spell)
	if GemRB.GetVar ("Type")!=-1:
		GemRB.SetVar ("ActionLevel", 0)
		#init spell list
		GemRB.SpellCast (pc, -1, 0)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	#equipment index
	GemRB.UseItem (pc, -1, Item, -1)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

SelectionChangeHandler = None

def SetSelectionChangeHandler (handler):
	"""Updates the selection handler."""

	global SelectionChangeHandler

	# Switching from walking to non-walking environment:
	# set the first selected PC in walking env as a selected
	# in nonwalking env
	#if (not SelectionChangeHandler) and handler:
	if (not SelectionChangeHandler) and handler and (not GUICommon.NextWindowFn):
		sel = GemRB.GameGetFirstSelectedPC ()
		if not sel:
			sel = 1
		GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	# redraw selection on change main selection | single selection
	SelectionChanged ()
	return

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()
	return

def OpenPortraitWindow (needcontrols):
	global PortraitWindow

	#take care, this window is different in how/iwd
	if GUICommon.HasHOW() and needcontrols:
		PortraitWindow = Window = GemRB.LoadWindow (26)
	else:
		PortraitWindow = Window = GemRB.LoadWindow (1)

	if needcontrols:
		# 1280 and higher don't have this control
		if Window.HasControl (8):
			Button=Window.GetControl (8)
			if GUICommon.GameIsIWD():
				# Rest (iwd)
				Button.SetTooltip (11942)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)
			else:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MinimizePortraits)
		else:
			if GUICommon.HasHOW():
				# Rest (how)
				pos = GemRB.GetSystemVariable (SV_HEIGHT) - 37
				Window.CreateButton (8, 6, pos, 55, 37)
				Button = Window.GetControl (8)
				Button.SetSprites ("GUIRSBUT", 0,0,1,0,0)
				Button.SetTooltip (11942)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

				pos = pos - 37
				Window.CreateButton (6, 6, pos, 27, 36)

		# AI
		Button = Window.GetControl (6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		Button.SetSprites ("GUIBTACT", 0, 46, 47, 48, 49)
		GSFlags = GemRB.GetMessageWindowSize ()&GS_PARTYAI

		GemRB.SetVar ("AI", GSFlags)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, AIPress)
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		Button.SetVarAssoc ("AI", GS_PARTYAI)
		if GSFlags:
			Button.SetTooltip (15917)
		else:
			Button.SetTooltip (15918)

		#Select All
		if GUICommon.HasHOW():
			Window.CreateButton (7, 33, pos, 27, 36)
			Button = Window.GetControl (7)
			Button.SetSprites ("GUIBTACT", 0, 50, 51, 50, 51)
		else:
			Button = Window.GetControl (7)
		Button.SetTooltip (10485)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectAllOnPress)
	else:
		# Rest
		if Window.HasControl(6):
			Button = Window.GetControl (6)
			Button.SetTooltip (11942)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		Button.SetFont ("STATES2")
		Button.SetVarAssoc ("PressedPortrait", i+1)

		if (needcontrols):
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, GUIINV.OpenInventoryWindowClick)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, PortraitButtonOnPress)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitButtonOnPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, PortraitButtonOnShiftPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnDropItemToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT, OnDropPortraitToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, PortraitButtonOnDrag)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, PortraitButtonOnMouseEnter)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, PortraitButtonOnMouseLeave)

		if GUICommon.GameIsIWD1():
			# overlay a label, so we can display the hp with the correct font
			Button.CreateLabelOnButton(100+i, "NUMFONT", IE_GUI_BUTTON_ALIGN_TOP|IE_GUI_BUTTON_ALIGN_LEFT)

		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = PortraitWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for portid in range (PARTY_SIZE):
		Button = Window.GetControl (portid)
		pic = GemRB.GetPlayerPortrait (portid+1, 1)
		if Inventory and pc != portid+1:
			pic = None

		if pic and GemRB.GetPlayerStat(portid+1, IE_STATE_ID) & STATE_DEAD:
			import GUISTORE
			# dead pcs are hidden in all stores but temples
			if GUISTORE.StoreWindow and not GUISTORE.StoreHealWindow:
				pic = None

		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue

		Button.SetFlags (IE_GUI_BUTTON_PICTURE| IE_GUI_BUTTON_HORIZONTAL| \
				IE_GUI_BUTTON_ALIGN_LEFT| IE_GUI_BUTTON_ALIGN_TOP| \
				IE_GUI_BUTTON_DRAGGABLE|IE_GUI_BUTTON_MULTILINE, OP_SET)

		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetPicture (pic, "NOPORTSM")
		ratio_str = GUICommon.SetupDamageInfo (portid+1, Button)

		#add effects on the portrait
		effects = GemRB.GetPlayerStates (portid+1)
		states = ""
		for col in range(len(effects)):
			states = effects[col:col+1] + states
			if col % 3 == 2: states = "\n" + states
		for x in range(3 - (len(effects)/3)):
			states = "\n" + states
		states = "\n" + states

		# character - 1 == bam cycle
		# blank space
		if GUICommon.GameIsBG2():
			flag = blank = chr(238)
			talk = 154
			store = 155
		else:
			# iwd is missing all of these, including the blank (didn't display a talking icon though)
			# TODO: add another string tag to make glyphs 100% transparent?
			flag = blank = chr(33)
			talk = store = 37

		if not GUICommon.GameIsIWD1():
			# shopping icon
			if pc==portid+1:
				if GemRB.GetStore()!=None:
					flag = chr(store)
			# talk icon
			if GemRB.GameGetSelectedPCSingle(1)==portid+1:
				flag = chr(talk)

		if LUCommon.CanLevelUp (portid+1):
			states = flag+blank+chr(255) + states
		else:
			if GUICommon.GameIsIWD1():
				HPLabel = Window.GetControl (100+portid)
				HPLabel.SetText (ratio_str) # TODO: color depending on the ratio
			else:
				states = flag+blank+blank + states
		Button.SetText(states)
	return

def PortraitButtonOnDrag ():
	global DraggedPortrait

	#they start from 1
	DraggedPortrait = GemRB.GetVar ("PressedPortrait")
	GemRB.DragItem (DraggedPortrait, -1, "")
	return

def PortraitButtonOnPress ():
	"""Selects the portrait individually."""

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	if GemRB.GameControlGetTargetMode() != TARGET_MODE_NONE:
		GemRB.ActOnPC (i)
		return

	if (not SelectionChangeHandler):
		if GemRB.GameIsPCSelected (i):
			GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR)
		GemRB.GameSelectPC (i, True, SELECT_REPLACE)
	else:
		GemRB.GameSelectPCSingle (i)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def PortraitButtonOnShiftPress ():
	"""Handles selecting multiple portaits with shift."""

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	if (not SelectionChangeHandler):
		sel = GemRB.GameIsPCSelected (i)
		sel = not sel
		GemRB.GameSelectPC (i, sel)
	else:
		GemRB.GameSelectPCSingle (i)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def SelectionChanged ():
	"""Ran by the Game class when a PC selection is changed."""

	global PortraitWindow

	if not PortraitWindow:
		return

	GemRB.SetVar ("ActionLevel", 0)
	if (not SelectionChangeHandler):
		UpdateActionsWindow ()
		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		#update mage school
		GemRB.SetVar ("MAGESCHOOL", 0)
		Kit = GUICommon.GetKitIndex (sel)
		if Kit and CommonTables.KitList.GetValue (Kit, 7) == 1:
			MageTable = GemRB.LoadTable ("magesch")
			GemRB.SetVar ("MAGESCHOOL", MageTable.FindValue (3, CommonTables.KitList.GetValue (Kit, 6) ) )

		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, i + 1 == sel)
	import CommonWindow
	CommonWindow.CloseContainerWindow()
	return

def PortraitButtonOnMouseEnter ():
	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	GemRB.GameControlSetLastActor( i )
	if GemRB.IsDraggingItem()==2:
		if DraggedPortrait != None:
			GemRB.SwapPCs (DraggedPortrait, i)
			GemRB.SetVar ("PressedPortrait", DraggedPortrait)
			DraggedPortrait = i
			GemRB.SetTimedEvent (CheckDragging, 1)
		else:
			OnDropPortraitToPC()
		return

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i-1)
		Button.EnableBorder (FRAME_PC_TARGET, 1)
	return

def OnDropPortraitToPC ():
	GemRB.SetVar ("PressedPortrait",0)
	GemRB.DragItem (0, -1, "")
	DraggedPortrait = None
	return

def CheckDragging():
	"""Contains portrait dragging in case of mouse out-of-range."""

	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")
	if not i:
		GemRB.DragItem (0, -1, "")

	if GemRB.IsDraggingItem()!=2:
		DraggedPortrait = None
	return

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ("PressedPortrait")
	if not i:
		return

	Button = PortraitWindow.GetControl (i-1)
	Button.EnableBorder (FRAME_PC_TARGET, 0)
	GemRB.SetVar ("PressedPortrait", 0)
	GemRB.SetTimedEvent (CheckDragging, 1)
	return

def ActionStopPressed ():
	for i in GemRB.GetSelectedActors():
		GemRB.ClearActions (i)
	return

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK,GA_NO_DEAD|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK,GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_DEFEND,GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD|GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def MinimizePortraits():
	GemRB.GameSetScreenFlags(GS_PORTRAITPANE, OP_OR)

def OpenWaitForDiscWindow ():
	global DiscWindow

	if DiscWindow:
		GemRB.HideGUI ()
		if DiscWindow:
			DiscWindow.Unload ()
		GemRB.SetVar ("OtherWindow", -1)
		# ...LoadWindowPack()
		EnableAnimatedWindows ()
		DiscWindow = None
		GemRB.UnhideGUI ()
		return

	try:
		GemRB.HideGUI ()
	except:
		pass

	GemRB.LoadWindowPack ("GUIID")
	DiscWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", Window.ID)
	label = DiscWindow.GetControl (0)

	disc_num = GemRB.GetVar ("WaitForDisc")
	#disc_path = GemRB.GetVar ("WaitForDiscPath")
	disc_path = 'XX:'

	text = GemRB.GetString (31483) + " " + str (disc_num) + " " + GemRB.GetString (31569) + " " + disc_path + "\n" + GemRB.GetString (49152)
	label.SetText (text)
	DisableAnimatedWindows ()
	# 31483 - Please place PS:T disc number
	# 31568 - Please place the PS:T DVD
	# 31569 - in drive
	# 31570 - Wrong disc in drive
	# 31571 - There is no disc in drive
	# 31578 - No disc could be found in drive. Please place Disc 1 in drive.
	# 49152 - To quit the game, press Alt-F4

	try:
		GemRB.UnhideGUI ()
	except:
		DiscWindow.SetVisible (WINDOW_VISIBLE)

def CheckLevelUp(pc):
	GemRB.SetVar ("CheckLevelUp"+str(pc), LUCommon.CanLevelUp (pc))

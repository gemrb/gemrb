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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


# GUICommonWindows.py - functions to open common
# windows in lower part of the screen
###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_modal import *
from ie_action import *
from ie_slots import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

PortraitWindow = None
OptionsWindow = None
ActionsWindow = None

def SetupMenuWindowControls (Window, Gears, ReturnToGame):
	global OptionsWindow

	OptionsWindow = Window
	# Return to Game
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetTooltip (Window, Button, 16313)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, ReturnToGame)

	# Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetTooltip (Window, Button, 16310)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Journal
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetTooltip (Window, Button, 16308)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	# Inventory
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 16307)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Records
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 16306)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 4)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Mage
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetTooltip (Window, Button, 16309)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 5)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageWindow")

	# Priest
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetTooltip (Window, Button, 14930)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 6)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestWindow")

	# Options
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetTooltip (Window, Button, 16311)
	GemRB.SetVarAssoc (Window, Button, "SelectedWindow", 7)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Multi player team setup?
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetTooltip (Window, Button, 13902)
	
	if Gears:
		# Pendulum, gears, sun/moon dial  (time)
		# FIXME: display all animations: CPEN, CGEAR, CDIAL
		Button = GemRB.GetControl (Window, 9)
		GemRB.SetAnimation (Window, Button, "CPEN")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
		rb = 11
	else:
		rb = 9

	# Rest
	Button = GemRB.GetControl (Window, rb)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RestPress")
	GemRB.SetTooltip (Window, Button, 11942)

def MarkMenuButton (WindowIndex):
	Pressed = GemRB.GetVar ("SelectedWindow")

	for button in range (9):
		Button = GemRB.GetControl (WindowIndex, button)
		GemRB.SetButtonState (WindowIndex, Button, IE_GUI_BUTTON_ENABLED)

	if Pressed: # don't draw the selection when returning to the game
		GemRB.SetButtonState (WindowIndex, Pressed, IE_GUI_BUTTON_SELECTED)

def AIPress ():
	Button = GemRB.GetControl (PortraitWindow, 6)
	AI = GemRB.GetMessageWindowSize () & GS_PARTYAI

	if AI:
		GemRB.GameSetScreenFlags(GS_PARTYAI, OP_NAND)
		GemRB.SetTooltip (PortraitWindow, Button, 15918)
		GemRB.SetVar ("AI", 0)
	else:
		GemRB.GameSetScreenFlags(GS_PARTYAI, OP_OR)
		GemRB.SetTooltip (PortraitWindow, Button, 15917)
		GemRB.SetVar ("AI", GS_PARTYAI)
	return

def RestPress ():
	GemRB.RestParty(0,0,0)
	return

def EmptyControls ():
	global ActionsWindow

	GemRB.SetVar ("ActionLevel", 0)
	Window = ActionsWindow
	for i in range (12):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		GemRB.SetButtonPicture (Window, Button, "")
	return

def SelectFormationPreset ():
	GemRB.GameSetFormation (GemRB.GetVar ("Value"), GemRB.GetVar ("Formation") )
	GroupControls ()
	return

def SetupFormation ():
	global ActionsWindow

	Window = ActionsWindow
	for i in range(12):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NORMAL, OP_SET)
		GemRB.SetButtonSprites (Window, Button, "GUIBTBUT",0,0,1,2,3)
		GemRB.SetButtonBAM (Window, Button, "FORM%x"%i,0,0,-1)
		GemRB.SetVarAssoc (Window, Button, "Value", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectFormationPreset")
	return

def SelectFormation ():
	GemRB.GameSetFormation ( GemRB.GetVar ("Formation") )
	return

def GroupControls ():
	global ActionsWindow

	GemRB.SetVar ("ActionLevel", 0)
	Window = ActionsWindow
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetActionIcon (Window, Button, 7)
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetActionIcon (Window, Button, 15)
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetActionIcon (Window, Button, 21)
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetActionIcon (Window, Button, -1)
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetActionIcon (Window, Button, -1)
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetActionIcon (Window, Button, -1)
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetActionIcon (Window, Button, -1)
	GemRB.SetVar ("Formation", GemRB.GameGetFormation ())
	for i in range (5):
		Button = GemRB.GetControl (Window, 7+i)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		idx = GemRB.GameGetFormation (i)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
		GemRB.SetButtonSprites (Window, Button, "GUIBTBUT",0,0,1,2,3)
		GemRB.SetButtonBAM (Window, Button, "FORM%x"%idx,0,0,-1)
		GemRB.SetVarAssoc (Window, Button, "Formation", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectFormation")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "SetupFormation")
		str = GemRB.GetString (4935)
		GemRB.SetTooltip (Window, Button, "F%d - %s"%(8+i,str) )
	return

def OpenActionsWindowControls (Window):
	global ActionsWindow

	ActionsWindow = Window
	# Gears (time) when options pane is down
	Button = GemRB.GetControl (Window, 62)
	# FIXME: display all animations
	GemRB.SetAnimation (Window, Button, "CPEN")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)
	UpdateActionsWindow ()
	return

def UpdateActionsWindow ():
	global ActionsWindow, PortraitWindow, OptionsWindow
	global level, TopIndex

	if ActionsWindow == -1:
		return

	if ActionsWindow == None:
		return

	#fully redraw the side panes to cover the actions window
	#do this only when there is no 'otherwindow'
	if GemRB.GetVar ("OtherWindow") != -1:
		if PortraitWindow:
			GemRB.InvalidateWindow (PortraitWindow)
		if OptionsWindow:
			GemRB.InvalidateWindow (OptionsWindow)

	pc = 0
	for i in range (PARTY_SIZE):
		if GemRB.GameIsPCSelected (i+1):
			if pc == 0:
				pc = i+1
			else:
				pc = -1
				break

	#setting up the disabled button overlay (using the second border slot)
	for i in range (12):
		Button = GemRB.GetControl (ActionsWindow, i)
		GemRB.SetButtonBorder (ActionsWindow, Button, 1, 0, 0, 0, 0, 50,30,10,120, 0, 1)
		GemRB.SetButtonFont (ActionsWindow, Button, "NUMBER")
		GemRB.SetText (ActionsWindow, Button, "")

	if pc == 0:
		EmptyControls ()
		return
	if pc == -1:
		GroupControls ()
		return
	#this is based on class

	level = GemRB.GetVar ("ActionLevel")
	TopIndex = GemRB.GetVar ("TopIndex")
	if level == 0:
		GemRB.SetupControls (ActionsWindow, pc)
	elif level == 1:
		GemRB.SetupEquipmentIcons(ActionsWindow, pc, TopIndex)
	elif level == 2: #spells
		GemRB.SetVar ("Type", 3)
		GemRB.SetupSpellIcons(ActionsWindow, pc, 3, TopIndex)
	elif level == 3: #innates
		GemRB.SetVar ("Type", 4)
		GemRB.SetupSpellIcons(ActionsWindow, pc, 4, TopIndex)
	return

def OpenFloatMenuWindow ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL )

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_TALK)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_ATTACK)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_DEFEND)

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_PICK)

def ActionQWeaponPressed (which):
	pc = GemRB.GameGetFirstSelectedPC ()

	if GemRB.GetEquippedQuickSlot (pc,1)==which and not (GemRB.GameControlGetTargetMode() &TARGET_MODE_ATTACK):
		GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_ATTACK)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ALL)
		GemRB.SetEquippedQuickSlot (pc, which)

	GemRB.SetupControls (ActionsWindow, pc)
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

def ActionStopPressed ():
	for i in range (PARTY_SIZE):
		if GemRB.GameIsPCSelected(i + 1):
			GemRB.ClearAction(i + 1)
	return

#no check needed because the button wouldn't be drawn if illegal
def ActionLeftPressed ():
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
	pc = GemRB.GameGetFirstSelectedPC ()
	TopIndex = GemRB.GetVar ("TopIndex")
	Type = GemRB.GetVar ("Type")
	Max = GemRB.GetMemorizedSpellsCount(pc, Type)
	TopIndex += 10
	if TopIndex > Max - 10:
		if Max>10:
			TopIndex = Max-10
		else:
			TopIndex = 0

	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

def ActionBardSongPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetModalState (pc, MS_BATTLESONG)
	UpdateActionsWindow ()
	return

def ActionSearchPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetModalState (pc, MS_DETECTTRAPS)
	UpdateActionsWindow ()
	return

def ActionStealthPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetModalState (pc, MS_STEALTH)
	UpdateActionsWindow ()
	return
	
def ActionTurnPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.SetModalState (pc, MS_TURNUNDEAD)
	UpdateActionsWindow ()
	return
	
def ActionUseItemPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 1)
	UpdateActionsWindow ()
	return

def ActionCastPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 2)
	UpdateActionsWindow ()
	return

def ActionQItemPressed (action):
	pc = GemRB.GameGetFirstSelectedPC ()
	#quick slot
	GemRB.UseItem (pc, -2, action)
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

def ActionInnatePressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 3)
	UpdateActionsWindow ()
	return

def SpellPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_CAST)
	Spell = GemRB.GetVar ("Spell")
	Type = GemRB.GetVar ("Type")
	GemRB.SpellCast (pc, Type, Spell)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_ALL | TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	#equipment index
	GemRB.UseItem (pc, -1, Item)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def GetKitIndex (actor):
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	KitTable = GemRB.LoadTable ("kitlist")
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)
	KitIndex = 0

	if Kit&0xc000ffff == 0x40000000:
		print "Kit value: 0x%04X"%Kit
		KitIndex = Kit>>16 & 0xfff

	#looking for kit by the usability flag
	if KitIndex == 0:
		KitIndex = GemRB.FindTableValue (KitTable, 6, Kit)
		if KitIndex == -1:
			KitIndex = 0
		# needed so barbarians don't override other kits
		elif Class != GemRB.GetTableValue (KitTable, KitIndex, 7):
			print "KitIndex before hack", KitIndex
			KitIndex = 0

	return KitIndex

def GetActorClassTitle (actor):
	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)

	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	Class = GemRB.FindTableValue ( ClassTable, 5, Class )
	KitTable = GemRB.LoadTable ("kitlist")
	KitIndex = GetKitIndex (actor)

	if ClassTitle == 0:
		if KitIndex == 0:
			ClassTitle=GemRB.GetTableValue (ClassTable, Class, 2)
		else:
			ClassTitle=GemRB.GetTableValue (KitTable, KitIndex, 2)

	if ClassTitle == "*":
		return 0
	return ClassTitle

def GetActorPaperDoll (actor):
	PortraitTable = GemRB.LoadTable ("PDOLLS")
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "LEVEL%d" %(level+1)
	return GemRB.GetTableValue (PortraitTable, row, which)

SelectionChangeHandler = None

def SetSelectionChangeHandler (handler):
	global SelectionChangeHandler

	# Switching from walking to non-walking environment:
	# set the first selected PC in walking env as a selected
	# in nonwalking env
	if (not SelectionChangeHandler) and handler:
		sel = GemRB.GameGetFirstSelectedPC ()
		if not sel:
			sel = 1
		GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	# redraw selection on change main selection | single selection
	SelectionChanged ()

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()


def OpenPortraitWindow (needcontrols):
	global PortraitWindow

	PortraitWindow = Window = GemRB.LoadWindow (1)

	if needcontrols:
		Button=GemRB.GetControl (Window, 8)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MinimizePortraits")

		# AI
		Button = GemRB.GetControl (Window, 6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		GemRB.SetButtonSprites (Window, Button, "GUIBTACT",0,48,47,46,49)
		GSFlags = GemRB.GetMessageWindowSize ()&GS_PARTYAI
		
		GemRB.SetVar ("AI", GSFlags)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "AIPress")
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
		GemRB.SetVarAssoc (Window, Button, "AI", 1)
		if GSFlags:
			GemRB.SetTooltip (PortraitWindow, Button, 15917)
		else:
			GemRB.SetTooltip (PortraitWindow, Button, 15918)

		#Select All
		Button = GemRB.GetControl (Window, 7)
		GemRB.SetTooltip (Window, Button, 10485)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")

	for i in range (PARTY_SIZE):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "PressedPortrait", i)
		if (needcontrols):
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenInventoryWindowClick")
		else:
			GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "PortraitButtonOnPress")

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_ENTER_BUTTON, "PortraitButtonOnMouseEnter")
		GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "PortraitButtonOnMouseLeave")

		GemRB.SetButtonBorder (Window, Button, FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		GemRB.SetButtonBorder (Window, Button, FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	Window = PortraitWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for i in range (PARTY_SIZE):
		Button = GemRB.GetControl (Window, i)
		pic = GemRB.GetPlayerPortrait (i+1, 1)
		if Inventory and pc!=i+1:
			pic = None

		if not pic:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
			GemRB.SetText (Window, Button, "")
			GemRB.SetTooltip (Window, Button, "")
			continue

		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT|IE_GUI_BUTTON_HORIZONTAL, OP_SET)
		GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_ENABLED)
		GemRB.SetButtonPicture (Window, Button, pic, "NOPORTSM")
		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)
		state = GemRB.GetPlayerStat (i+1, IE_STATE_ID)

		if (hp_max<1):
			 ratio = 0.0
		else:
			 ratio = (hp+0.0) / hp_max

		if hp<1 or (state & STATE_DEAD):
			GemRB.SetButtonOverlay (Window, Button, ratio, 64,64,64,200, 64,64,64,200)
		else:
			GemRB.SetButtonOverlay (Window, Button, ratio, 255,0,0,200, 128,0,0,200)
		GemRB.SetTooltip (Window, Button, GemRB.GetPlayerName (i+1, 1) + "\n%d/%d" %(hp, hp_max))


def PortraitButtonOnPress ():
	i = GemRB.GetVar ("PressedPortrait")

	if (not SelectionChangeHandler):
		if GemRB.GameIsPCSelected (i+1):
			GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR)
		GemRB.GameSelectPC (i + 1, True, SELECT_REPLACE)
	else:
		GemRB.GameSelectPCSingle (i + 1)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def PortraitButtonOnShiftPress ():
	i = GemRB.GetVar ("PressedPortrait")

	if (not SelectionChangeHandler):
		sel = GemRB.GameIsPCSelected (i + 1)
		sel = not sel
		GemRB.GameSelectPC (i + 1, sel)
	else:
		GemRB.GameSelectPCSingle (i + 1)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)
	return

# Run by Game class when selection was changed
def SelectionChanged ():
	global PortraitWindow

	GemRB.SetVar ("ActionLevel", 0)
	if (not SelectionChangeHandler):
		UpdateActionsWindow ()
		for i in range (PARTY_SIZE):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		for i in range (PARTY_SIZE):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, i + 1 == sel)
	return

def PortraitButtonOnMouseEnter ():
	i = GemRB.GetVar ("PressedPortrait")
	if GemRB.IsDraggingItem ():
		Button = GemRB.GetControl (PortraitWindow, i)
		GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_TARGET, 1)
	return

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ("PressedPortrait")
	Button = GemRB.GetControl (PortraitWindow, i)
	GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_TARGET, 0)
	return

def GetSavingThrow (SaveName, row, level):
	SaveTable = GemRB.LoadTable (SaveName)
	tmp = GemRB.GetTableValue (SaveTable, level)
	GemRB.UnloadTable (SaveName)
	return tmp

def SetupSavingThrows (pc):
	level1 = GemRB.GetPlayerStat (pc, IE_LEVEL) - 1
	if level1 > 20:
		level1 = 20
	level2 = GemRB.GetPlayerStat (pc, IE_LEVEL2) - 1
	if level2 > 20:
		level2 = 20
	Class = GemRB.GetPlayerStat (pc, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")

	Race = GemRB.GetPlayerStat (pc, IE_RACE)
	RaceTable = GemRB.LoadTable ("races")
	
	Class = GemRB.FindTableValue (ClassTable, 5, Class)
	Multi = GemRB.GetTableValue (ClassTable, Class, 4)

	Race = GemRB.FindTableValue (RaceTable, 3, Race)
	RaceSaveTableName = GemRB.GetTableValue (RaceTable, Race, 4)

	#todo fix multi class
	if Multi:
		if Class == 7:
			#fighter/mage
			Class = GemRB.FindTableValue (ClassTable, 5, 1)
		else:
			#fighter/thief
			Class = GemRB.FindTableValue (ClassTable, 5, 4)
		SaveName2 = GemRB.GetTableValue (ClassTable, Class, 3)
		Class = 0 #fighter

	SaveName1 = GemRB.GetTableValue (ClassTable, Class, 3)

	for row in range (5):
		tmp1 = GetSavingThrow (SaveName1, row, level1)
		if Multi:
			tmp2 = GetSavingThrow (SaveName2, row, level2)
			if tmp2<tmp1:
				tmp1=tmp2
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, tmp1)
	if RaceSaveTableName!="*":
		Con = GemRB.GetPlayerStat (pc, IE_CON)
		RaceSaveTable = GemRB.LoadTable (RaceSaveTableName)
		for row in range (5):
			tmp1 = GemRB.GetPlayerStat (pc, IE_SAVEVSDEATH+row)
			tmp1 += GemRB.GetTableValue (RaceSaveTable, row, Con)
			GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, tmp1)
		GemRB.UnloadTable (RaceSaveTable)
	return

def SetEncumbranceLabels (Window, Label, Label2, pc):
	# encumbrance
	# Loading tables of modifications
	Table = GemRB.LoadTable ("strmod")
	TableEx = GemRB.LoadTable ("strmodex")
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	max_encumb = GemRB.GetTableValue (Table, sstr, 3) + GemRB.GetTableValue (TableEx, ext_str, 3)
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Label = GemRB.GetControl (Window, 0x10000043)
	GemRB.SetText (Window, Label, str (encumbrance) + ":")

	Label2 = GemRB.GetControl (Window, 0x10000044)
	GemRB.SetText (Window, Label2, str (max_encumb) + ":")
	ratio = (0.0 + encumbrance) / max_encumb
	if ratio > 1.0:
		GemRB.SetLabelTextColor (Window, Label, 255, 0, 0)
		GemRB.SetLabelTextColor (Window, Label2, 255, 0, 0)
	elif ratio > 0.8:
		GemRB.SetLabelTextColor (Window, Label, 255, 255, 0)
		GemRB.SetLabelTextColor (Window, Label2, 255, 0, 0)
	else:
		GemRB.SetLabelTextColor (Window, Label, 255, 255, 255)
		GemRB.SetLabelTextColor (Window, Label2, 255, 0, 0)
	return

def DebugInventorySlots(func=""):
	print
	print func
	pc = GemRB.GameGetSelectedPCSingle ()
	for i in range(16):
		print 2**i, GemRB.GetSlots (pc, 2**i, 1)
	print SLOT_ANY, GemRB.GetSlots (pc, SLOT_ANY, 1)
	print

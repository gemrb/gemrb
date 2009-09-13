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
# $Id$


# GUICommonWindows.py - functions to open common windows in lower part of the screen

import GemRB
from GUIDefines import *
from GUICommon import *
from ie_stats import *
from ie_modal import *
from ie_action import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

PortraitWindow = None
OptionsWindow = None
DraggedPortrait = None

def SetupMenuWindowControls (Window, Gears, ReturnToGame):
	global OptionsWindow

	OptionsWindow = Window
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReturnToGame)

	# Spellbook
	Button = Window.GetControl (4)
	Button.SetTooltip (16309)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSpellBookWindow")

	# Inventory
	Button = Window.GetControl (5)
	Button.SetTooltip (16307)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Journal
	Button = Window.GetControl (6)
	Button.SetTooltip (16308)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")
	# Map
	Button = Window.GetControl (7)
	Button.SetTooltip (16310)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Records
	Button = Window.GetControl (8)
	Button.SetTooltip (16306)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Options
	Button = Window.GetControl (9)
	Button.SetTooltip (16311)
	Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	Button.SetVarAssoc ("SelectedWindow", 7)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Gears
	Button = Window.GetControl (10)
	Button.SetAnimation ("CGEAR")
	Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	if Gears:
		# Select All
		Button = Window.GetControl (11)
		Button.SetTooltip (10485)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")
		# Rest
		Button = Window.GetControl (12)
		Button.SetTooltip (11942)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RestPress")

		# Character Arbitration
		Button = Window.GetControl (13)
		Button.SetTooltip (16312)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CharacterWindow")
	return

def AIPress ():
	print "AIPress"
	return

def EmptyControls ():
	global PortraitWindow

	Window = PortraitWindow
	for i in range (12):
		Button = Window.GetControl (i+6)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetPicture ("")
	return

def SelectFormationPreset ():
	GemRB.GameSetFormation ( GemRB.GetVar ("Value"), GemRB.GetVar ("Formation") )
	GroupControls ()
	return

def SetupFormation ():
	global PortraitWindow

	Window = PortraitWindow
	for i in range (12):
		Button = Window.GetControl (i+6)
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NORMAL, OP_SET
)
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%i,0,0,-1)
		Button.SetVarAssoc ("Value", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectFormationPreset")
	return

def GroupControls ():
	global PortraitWindow

	Window = PortraitWindow
	Button = Window.GetControl (6)
	Button.SetActionIcon (7)
	Button = Window.GetControl (7)
	Button.SetActionIcon (15)
	Button = Window.GetControl (8)
	Button.SetActionIcon (21)
	Button = Window.GetControl (9)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (10)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (11)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (12)
	Button.SetActionIcon (-1)
	GemRB.SetVar ("Formation", GemRB.GameGetFormation ())
	for i in range (5):
		Button = Window.GetControl (13+i)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		idx = GemRB.GameGetFormation (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%idx,0,0,-1)
		Button.SetVarAssoc ("Formation", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectFormation")
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "SetupFormation")
		str = GemRB.GetString (4935)
		Button.SetTooltip ("F%d - %s"%(8+i,str) )
	return

def UpdateActionsWindow ():
	global PortraitWindow
	global level, TopIndex

	if PortraitWindow == -1:
		return

	if PortraitWindow == None:
		return

	pc = 0
	#do this only when there is no 'otherwindow'
	if GemRB.GetVar ("OtherWindow") != -1:
		return

	for i in range (PARTY_SIZE):
		if GemRB.GameIsPCSelected (i+1):
			if pc == 0:
				pc = i+1
			else:
				pc = -1
				break

	if pc == 0:
		EmptyControls ()
		return
	if pc == -1:
		GroupControls ()
		return

	level = GemRB.GetVar ("ActionLevel")
	TopIndex = GemRB.GetVar ("TopIndex")
	if level == 0:
		PortraitWindow.SetupControls (pc, 6)
	elif level == 1:
		PortraitWindow.SetupEquipmentIcons (pc, TopIndex, 6)
	elif level == 2: #spells
		GemRB.SetVar ("Type", 3)
		PortraitWindow.SetupSpellIcons (pc, 3, TopIndex, 6)
	elif level == 3: #innates
		GemRB.SetVar ("Type", 4)
		PortraitWindow.SetupSpellIcons (pc, 4, TopIndex, 6)
	return

def ActionQWeaponPressed (which):
	global PortraitWindow

	pc = GemRB.GameGetFirstSelectedPC ()

	if GemRB.GetEquippedQuickSlot (pc,1)==which and GemRB.GameControlGetTargetMode () != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which)

	PortraitWindow.SetupControls (pc, 6)
	UpdateActionsWindow ()
	return

def ActionQWeapon1Pressed ():
	ActionQWeaponPressed (0)

def ActionQWeapon2Pressed ():
	ActionQWeaponPressed (1)

def ActionQWeapon3Pressed ():
	ActionQWeaponPressed (2)

def ActionQWeapon4Pressed ():
	ActionQWeaponPressed (3)

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

def ActionRightPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()
	TopIndex = GemRB.GetVar ("TopIndex")
	Type = GemRB.GetVar ("Type")
	Max = GemRB.GetMemorizedSpellsCount (pc, Type)
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
	print "CASRRE"
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

def ActionSkillsPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 4)
	UpdateActionsWindow ()
	return

def SpellPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Spell = GemRB.GetVar ("Spell")
	Type = GemRB.GetVar ("Type")
	GemRB.SpellCast (pc, Type, Spell)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedPC ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	#equipment index
	GemRB.UseItem (pc, -1, Item)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def GetActorRaceTitle (actor):
	Table = GemRB.LoadTableObject ("races")
	RaceID = GemRB.GetPlayerStat (actor, IE_SUBRACE)
	if RaceID:
		RaceID += GemRB.GetPlayerStat (actor, IE_RACE)<<16
	else:
		RaceID = GemRB.GetPlayerStat (actor, IE_RACE)
	row = Table.FindValue (3, RaceID )
	RaceTitle = Table.GetValue (row, 2)
	return RaceTitle

# NOTE: this function is called with the primary classes
def GetKitIndex (actor, ClassIndex):
	ClassTable = GemRB.LoadTableObject ("classes")
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)

	KitIndex = -1
	ClassID = ClassTable.GetValue (ClassIndex, 2)
	# skip the primary classes
	for ci in range (10, ClassTable.GetRowCount ()):
		BaseClass = ClassTable.GetValue (ci, 3)
		if BaseClass == ClassID and Kit & ClassTable.GetValue (ci, 2):
			KitIndex = ci

	if KitIndex == -1:
		return 0

	return KitIndex

def GetActorClassTitle (actor, ClassIndex):
	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)
	if ClassTitle:
		return ClassTitle

	#Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTableObject ("classes")
	KitIndex = GetKitIndex (actor, ClassIndex)
	if KitIndex == 0:
		ClassTitle = ClassTable.GetValue (ClassIndex, 0)
	else:
		ClassTitle = ClassTable.GetValue (KitIndex, 0)

	if ClassTitle == "*":
		return 0
	return ClassTitle

# overriding the one in GUICommon, since we use a different table and animations
def GetActorPaperDoll (actor):
	PortraitTable = GemRB.LoadTableObject ("avatars")
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "AT_%d" %(level+1)
	return PortraitTable.GetValue (row, which)


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

def OpenPortraitWindow ():
	global PortraitWindow

	PortraitWindow = Window = GemRB.LoadWindowObject (1)

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, "PortraitButtonOnDrag")
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "PortraitButtonOnMouseEnter")
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, "PortraitButtonOnMouseLeave")

		Button.SetFlags (IE_GUI_BUTTON_ALIGN_TOP|IE_GUI_BUTTON_ALIGN_LEFT|IE_GUI_BUTTON_PICTURE, OP_SET)

		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)
		Button.SetVarAssoc ("PressedPortrait", i)
		Button.SetFont ('NUMFONT')

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	Window = PortraitWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		pic = GemRB.GetPlayerPortrait (i+1, 1)
		if Inventory and pc != i+1:
			pic = None
		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue

		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT|IE_GUI_BUTTON_HORIZONTAL|IE_GUI_BUTTON_DRAGGABLE, OP_SET)
		Button.SetPicture (pic, "NOPORTSM")

		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)
		Button.SetText ("%d/%d" %(hp, hp_max))
		Button.SetTooltip (GemRB.GetPlayerName (i+1, 1) + "\n%d/%d" %(hp, hp_max))

	return

def PortraitButtonOnDrag ():
	global DraggedPortrait

	#they start from 1
	DraggedPortrait = GemRB.GetVar ("PressedPortrait")+1
	GemRB.DragItem (DraggedPortrait, -1, "")
	return

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
	i = GemRB.GetVar ('PressedPortrait')

	if (not SelectionChangeHandler):
		sel = GemRB.GameIsPCSelected (i + 1)
		sel = not sel
		GemRB.GameSelectPC (i + 1, sel)
	else:
		GemRB.GameSelectPCSingle (i + 1)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		UpdateActionsWindow ()
		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, i + 1 == sel)
	return

def PortraitButtonOnMouseEnter ():
	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")

	if DraggedPortrait != None:
		GemRB.DragItem (0, -1, "")
		#this might not work
		GemRB.SwapPCs (DraggedPortrait, i+1)
		DraggedPortrait = None
		return

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i)
		Button.EnableBorder (FRAME_PC_TARGET, 1)
	return

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ("PressedPortrait")
	Button = PortraitWindow.GetControl (i)
	Button.EnableBorder (FRAME_PC_TARGET, 0)
	return

def CheckLevelUp(pc):
	GemRB.SetVar (CheckLevelUp+str(pc), CanLevelUp (pc))

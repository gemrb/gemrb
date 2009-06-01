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


# GUICommonWindows.py - functions to open common
# windows in lower part of the screen
###################################################

import GemRB
from math import ceil
from GUIDefines import *
from ie_stats import *
from ie_modal import *
from ie_action import *
from ie_slots import *
from GUICommon import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

PortraitWindow = None
OptionsWindow = None
ActionsWindow = None
DraggedPortrait = None

ClassTable = GemRB.LoadTableObject ("classes")
NextLevelTable = GemRB.LoadTableObject ("XPLEVEL")
KitListTable = GemRB.LoadTableObject ("kitlist")
ClassSkillsTable = GemRB.LoadTableObject ("clskills")
RaceTable = GemRB.LoadTableObject ("races")

# only used in SetEncumbranceLabels, but that is called very often
StrModTable = GemRB.LoadTableObject ("strmod")
StrModExTable = GemRB.LoadTableObject ("strmodex")

def SetupMenuWindowControls (Window, Gears, ReturnToGame):
	global OptionsWindow

	OptionsWindow = Window
	# Return to Game
	Button = Window.GetControl (0)
	Button.SetTooltip (16313)
	Button.SetVarAssoc ("SelectedWindow", 0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReturnToGame)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Map
	Button = Window.GetControl (1)
	Button.SetTooltip (16310)
	Button.SetVarAssoc ("SelectedWindow", 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Journal
	Button = Window.GetControl (2)
	Button.SetTooltip (16308)
	Button.SetVarAssoc ("SelectedWindow", 2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	# Inventory
	Button = Window.GetControl (3)
	Button.SetTooltip (16307)
	Button.SetVarAssoc ("SelectedWindow", 3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Records
	Button = Window.GetControl (4)
	Button.SetTooltip (16306)
	Button.SetVarAssoc ("SelectedWindow", 4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Mage
	Button = Window.GetControl (5)
	Button.SetTooltip (16309)
	Button.SetVarAssoc ("SelectedWindow", 5)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMageWindow")

	# Priest
	Button = Window.GetControl (6)
	Button.SetTooltip (14930)
	Button.SetVarAssoc ("SelectedWindow", 6)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenPriestWindow")

	# Options
	Button = Window.GetControl (7)
	Button.SetTooltip (16311)
	Button.SetVarAssoc ("SelectedWindow", 7)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Multi player team setup?
	Button = Window.GetControl (8)
	Button.SetTooltip (13902)

	if Gears:
		# Pendulum, gears, sun/moon dial (time)
		# FIXME: display all animations: CPEN, CGEAR, CDIAL
		Button = Window.GetControl (9)
		Label = Button.CreateLabelOnButton (0x10000009, "NORMAL", 0)

		Label.SetAnimation ("CPEN")
		Button.SetAnimation ("CGEAR")
		Button.SetBAM ("CDIAL", 0, 0)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GearsClicked")
		SetGamedaysAndHourToken()
		Button.SetTooltip(16041)
		rb = 11
	else:
		rb = 9

	# Rest
	Button = Window.GetControl (rb)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RestPress")
	Button.SetTooltip (11942)

	if PortraitWindow:
		UpdatePortraitWindow ()

def MarkMenuButton (WindowIndex):
	Pressed = WindowIndex.GetControl( GemRB.GetVar ("SelectedWindow") )

	for button in range (9):
		Button = WindowIndex.GetControl (button)
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	if Pressed: # don't draw the selection when returning to the game
		Pressed.SetState (IE_GUI_BUTTON_SELECTED)

def AIPress ():
	Button = PortraitWindow.GetControl (6)
	AI = GemRB.GetMessageWindowSize () & GS_PARTYAI

	if AI:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_NAND)
		Button.SetTooltip (15918)
		GemRB.SetVar ("AI", 0)
	else:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
		Button.SetTooltip (15917)
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
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetPicture ("")
	return

def SelectFormationPreset ():
	GemRB.GameSetFormation (GemRB.GetVar ("Value"), GemRB.GetVar ("Formation") )
	GroupControls ()
	return

def SetupFormation ():
	global ActionsWindow

	Window = ActionsWindow
	for i in range (12):
		Button = Window.GetControl (i)
		Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%i,0,0,-1)
		Button.SetVarAssoc ("Value", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectFormationPreset")
	return

def SelectFormation ():
	GemRB.GameSetFormation ( GemRB.GetVar ("Formation") )
	return

def GroupControls ():
	global ActionsWindow

	GemRB.SetVar ("ActionLevel", 0)
	Window = ActionsWindow
	Button = Window.GetControl (0)
	Button.SetActionIcon (7)
	Button = Window.GetControl (1)
	Button.SetActionIcon (15)
	Button = Window.GetControl (2)
	Button.SetActionIcon (21)
	Button = Window.GetControl (3)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (4)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (5)
	Button.SetActionIcon (-1)
	Button = Window.GetControl (6)
	Button.SetActionIcon (-1)
	GemRB.SetVar ("Formation", GemRB.GameGetFormation ())
	for i in range (5):
		Button = Window.GetControl (7+i)
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

def OpenActionsWindowControls (Window):
	global ActionsWindow

	ActionsWindow = Window
	# Gears (time) when options pane is down
	Button = Window.GetControl (62)
	Label = Button.CreateLabelOnButton (0x1000003e, "NORMAL", 0)

	# FIXME: display all animations
	Label.SetAnimation ("CPEN")
	Button.SetAnimation ("CGEAR")
	Button.SetBAM ("CDIAL", 0, 0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "GearsClicked")
	SetGamedaysAndHourToken()
	Button.SetTooltip(16041)
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
			PortraitWindow.Invalidate ()
		if OptionsWindow:
			OptionsWindow.Invalidate ()

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
		Button = ActionsWindow.GetControl (i)
		Button.SetBorder (1, 0, 0, 0, 0, 50,30,10,120, 0, 1)
		Button.SetFont ("NUMBER")
		Button.SetText ("")

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
		ActionsWindow.SetupControls (pc)
	elif level == 1:
		ActionsWindow.SetupEquipmentIcons(pc, TopIndex)
	elif level == 2: #spells
		GemRB.SetVar ("Type", 3)
		ActionsWindow.SetupSpellIcons(pc, 3, TopIndex)
	elif level == 3: #innates
		GemRB.SetVar ("Type", 4)
		ActionsWindow.SetupSpellIcons(pc, 4, TopIndex)
	return

def OpenFloatMenuWindow ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK,GA_NO_DEAD|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK,GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_DEFEND,GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD|GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionQWeaponPressed (which):
	pc = GemRB.GameGetFirstSelectedPC ()
	qs = GemRB.GetEquippedQuickSlot (pc,1)

	#38 is the magic slot
	if ((qs==which) or (qs==38)) and GemRB.GameControlGetTargetMode() != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which)

	ActionsWindow.SetupControls (pc)
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
			GemRB.ClearActions(i + 1)
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
	#Type is a bitfield if there is no level given
	#This is to make sure cleric/mages get all spells listed
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

def GetKitIndex (actor):
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)
	KitIndex = 0

	if Kit & 0xc000 == 0x4000:
		KitIndex = Kit & 0xfff

	# carefully looking for kit by the usability flag
	# since the barbarian kit id clashes with the no-kit value
	if KitIndex == 0 and Kit != 0x4000:
		KitIndex = KitListTable.FindValue (6, Kit)
		if KitIndex == -1:
			KitIndex = 0

	return KitIndex

def GetActorClassTitle (actor):
	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)

	if ClassTitle == 0:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = ClassTable.FindValue ( 5, Class )
		KitIndex = GetKitIndex (actor)
		Multi = ClassTable.GetValue (ClassIndex, 4)
		Dual = IsDualClassed (actor, 1)

		if Multi and Dual[0] == 0: # true multi class
			ClassTitle = ClassTable.GetValue (ClassIndex, 2)
			ClassTitle = GemRB.GetString (ClassTitle)
		else:
			if Dual[0]: # dual class
				# first (previous) kit or class of the dual class
				if Dual[0] == 1:
					ClassTitle = KitListTable.GetValue (Dual[1], 2)
				elif Dual[0] == 2:
					ClassTitle = ClassTable.GetValue (Dual[1], 2)
				ClassTitle = GemRB.GetString (ClassTitle) + " / "
				ClassTitle += GemRB.GetString (ClassTable.GetValue (Dual[2], 2))
			else: # ordinary class or kit
				if KitIndex:
					ClassTitle = KitListTable.GetValue (KitIndex, 2)
				else:
					ClassTitle = ClassTable.GetValue (ClassIndex, 2)
				ClassTitle = GemRB.GetString (ClassTitle)

	if ClassTitle == "*":
		return 0
	return ClassTitle

def GetActorPaperDoll (actor):
	PortraitTable = GemRB.LoadTableObject ("PDOLLS")
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "LEVEL%d" %(level+1)
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
	return

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()
	return

def OpenPortraitWindow (needcontrols):
	global PortraitWindow

	PortraitWindow = Window = GemRB.LoadWindowObject (1)

	if needcontrols:
		Button=Window.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "MinimizePortraits")

		# AI
		Button = Window.GetControl (6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		Button.SetSprites ("GUIBTACT", 0, 48, 47, 46, 49)
		GSFlags = GemRB.GetMessageWindowSize ()&GS_PARTYAI

		GemRB.SetVar ("AI", GSFlags)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "AIPress")
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		Button.SetVarAssoc ("AI", 1)
		if GSFlags:
			Button.SetTooltip (15917)
		else:
			Button.SetTooltip (15918)

		#Select All
		Button = Window.GetControl (7)
		Button.SetTooltip (10485)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		Button.SetFont ("STATES2")
		Button.SetVarAssoc ("PressedPortrait", i+1)
		if (needcontrols):
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "OpenInventoryWindowClick")
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "PortraitButtonOnPress")

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, "PortraitButtonOnDrag")
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "PortraitButtonOnMouseEnter")
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, "PortraitButtonOnMouseLeave")

		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	Window = PortraitWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for portid in range (PARTY_SIZE):
		Button = Window.GetControl (portid)
		pic = GemRB.GetPlayerPortrait (portid+1, 1)
		if Inventory and pc != portid+1:
			pic = None

		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue

		Button.SetFlags (IE_GUI_BUTTON_PICTURE| \
				IE_GUI_BUTTON_HORIZONTAL| \
				IE_GUI_BUTTON_ALIGN_LEFT| IE_GUI_BUTTON_ALIGN_TOP| \
				IE_GUI_BUTTON_DRAGGABLE|IE_GUI_BUTTON_MULTILINE, OP_SET)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetPicture (pic, "NOPORTSM")
		hp = GemRB.GetPlayerStat (portid+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (portid+1, IE_MAXHITPOINTS)
		state = GemRB.GetPlayerStat (portid+1, IE_STATE_ID)

		if (hp_max<1):
			ratio = 0.0
		else:
			ratio = (hp+0.0) / hp_max

		if hp<1 or (state & STATE_DEAD):
			Button.SetOverlay (ratio, 64,64,64,200, 64,64,64,200)
		else:
			Button.SetOverlay (ratio, 255,0,0,200, 128,0,0,200)
		Button.SetTooltip (GemRB.GetPlayerName (portid+1, 1) + "\n%d/%d" %(hp, hp_max))

		#add effects on the portrait
		effects = GemRB.GetPlayerStates (portid+1)
		states = ""
		for col in range(len(effects)):
			states = effects[col:col+1] + states
			if col % 3 == 2: states = "\n" + states
		for x in range(3 - (len(effects)/3)):
			states = "\n" + states
		states = "\n" + states

		# blank space
		flag = chr(238)

		# shopping icon
		if pc==portid+1:
			if GemRB.GetStore()!=None:
				flag = chr(155)
		# talk icon
		if GemRB.GameGetSelectedPCSingle(1)==portid+1:
			flag = chr(154)

		if CanLevelUp (portid+1):
			states = flag+chr(238)+chr(255) + states
		else:
			states = flag+chr(238)+chr(238) + states
		Button.SetText(states)
	return

def PortraitButtonOnDrag ():
	global DraggedPortrait

	#they start from 1
	DraggedPortrait = GemRB.GetVar ("PressedPortrait")
	GemRB.DragItem (DraggedPortrait, -1, "")
	return

def PortraitButtonOnPress ():
	i = GemRB.GetVar ("PressedPortrait")

	if not i:
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
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		#update mage school
		GemRB.SetVar ("MAGESCHOOL", 0)
		Kit = GetKitIndex (sel)
		if Kit and KitListTable.GetValue (Kit, 7) == 1:
			MageTable = GemRB.LoadTableObject ("magesch")
			GemRB.SetVar ("MAGESCHOOL", MageTable.FindValue (3, KitListTable.GetValue (Kit, 6) ) )

		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, i + 1 == sel)
	return

def PortraitButtonOnMouseEnter ():
	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")
	if GemRB.IsDraggingItem()==2:
		if DraggedPortrait != None:
			GemRB.SwapPCs (DraggedPortrait, i)
			GemRB.SetVar ("PressedPortrait", DraggedPortrait)
			DraggedPortrait = i
			GemRB.SetTimedEvent ("CheckDragging",1)
		else:
			OnDropItemToPC2()
		return

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i)
		Button.EnableBorder (FRAME_PC_TARGET, 1)
	return

def OnDropItemToPC2 ():
	GemRB.SetVar ("PressedPortrait",0)
	GemRB.DragItem (0, -1, "")
	DraggedPortrait = None
        return

#stop the portrait dragging if the mouse went on a trip
def CheckDragging():
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
	GemRB.SetTimedEvent ("CheckDragging",1)
	return

#Levels needs to be an array containing the level desired for each class
#this is potentially useful for leveling up 
def SetupSavingThrows (pc, Level=None):
	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	Race = GemRB.GetPlayerStat (pc, IE_RACE)

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#see if we can add racial bonuses to saves
	#default return is -1 NOT "*", so we convert always convert to str
	#I'm leaving the "*" just in case
	Race = RaceTable.GetRowName (RaceTable.FindValue (3, Race) )
	RaceSaveTableName = str(RaceTable.GetValue (Race, "SAVE") )
	RaceSaveTable = None
	if RaceSaveTableName != "-1" and RaceSaveTableName != "*":
		Con = GemRB.GetPlayerStat (pc, IE_CON, 1)-1
		RaceSaveTable = GemRB.LoadTableObject (RaceSaveTableName)
		if Con >= RaceSaveTable.GetRowCount ():
			Con = RaceSaveTable.GetRowCount ()-1

	#preload our tables to limit multi-classed lookups
	SaveTables = []
	for i in range (NumClasses):
		SaveName = ClassTable.GetValue (ClassTable.FindValue (5, Class[i]), 3, 0)
		SaveTables.append (GemRB.LoadTableObject (SaveName) )
	if not len (SaveTables):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = SaveTables[0].GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	#save the saves
	for row in range (5):
		CurrentSave = 20
		for i in range (NumClasses):
			#loop through each class and update the save value if we have
			#a better save
			TmpSave = SaveTables[i].GetValue (row, Levels[i])
			if TmpSave and (TmpSave < CurrentSave or i == 0):
				CurrentSave = TmpSave

		#add racial bonuses if applicable (small pc's)
		if RaceSaveTable:
			CurrentSave += RaceSaveTable.GetValue (row, Con)
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, CurrentSave)
	return

def SetupThaco (pc, Level=None):
	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL2)-1, \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)-1]
	else:
		Levels = []
		for level in Level:
			Levels.append (level-1)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	ThacoTable = GemRB.LoadTableObject ("THAC0")

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#assume Level is correct if passed
		if IsDualSwap(pc) and not Level:
			Levels = [Levels[1], Levels[0], Levels[2]]
	if NumClasses>len(Levels):
		return

	#make sure to limit the levels to the table allowable
	MaxLevel = ThacoTable.GetColumnCount ()-1
	for i in range (len(Levels)):
		if Levels[i] > MaxLevel:
			Levels[i] = MaxLevel

	CurrentThaco = GemRB.GetPlayerStat (pc, IE_THAC0, 1)
	NewThaco = 0
	for i in range (NumClasses):
		#loop through each class and update the save value if we have
		#a better thac0
		TmpThaco = ThacoTable.GetValue (Class[i]-1, Levels[i])
		if TmpThaco < CurrentThaco:
			NewThaco = 1
			CurrentThaco = TmpThaco

	#only update if we have a better thac0
	if NewThaco:
		GemRB.SetPlayerStat (pc, IE_THAC0, CurrentThaco)
	return

#LevelDiff should be an array containing the difference in levels of
#  each class. If left None, we assume that the character is starting
#  from level 0, and therefore LevelDiff = Level
def SetupLore (pc, LevelDiff=None):
	#storing levels as an array makes them easier to deal with
	if not LevelDiff:
		LevelDiffs = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		LevelDiffs = []
		for diff in LevelDiff:
			LevelDiffs.append (diff)

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
	LoreTable = GemRB.LoadTableObject ("lore")

	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#if LevelDiff is passed, we assume it is correct
		if IsDualSwap(pc) and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	if NumClasses>len(LevelDiffs):
		return

	#loop through each class and update the lore value if we have
	CurrentLore = GemRB.GetPlayerStat (pc, IE_LORE, 1)
	for i in range (NumClasses):
		#correct unlisted progressions
		ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class[i]) )
		if ClassName == "SORCERER":
			ClassName = "MAGE"
		elif ClassName == "MONK": #monks have a rate of 1, so this is arbitrary
			ClassName = "CLERIC"

		#add the lore from this class to the total lore
		TmpLore = LevelDiffs[i] * LoreTable.GetValue (ClassName, "RATE", 1)
		if TmpLore:
			CurrentLore += TmpLore

	#update our lore value
	GemRB.SetPlayerStat (pc, IE_LORE, CurrentLore)
	return

#Level should be an array containing the current level of the class.
#LevelDiff should be an array containing the difference in levels of
#  each class. If left None, we assume that the character is starting
#  from level 0, and therefore LevelDiff = Level.
def SetupHP (pc, Level=None, LevelDiff=None):
	#storing levels as an array makes them easier to deal with
	if not Level:
		Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		Levels = []
		for level in Level:
			Levels.append (level)
	if not LevelDiff:
		LevelDiffs = [GemRB.GetPlayerStat (pc, IE_LEVEL), \
			GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	else:
		LevelDiffs = []
		for diff in LevelDiff:
			LevelDiffs.append (diff)
	if len (Levels) != len (LevelDiffs):
		return

	#get some basic values
	Class = [GemRB.GetPlayerStat (pc, IE_CLASS)]
		
	#adjust the class for multi/dual chars
	Multi = IsMultiClassed (pc, 1)
	Dual = IsDualClassed (pc, 1)
	NumClasses = 1
	if Multi[0]>1: #get each of the multi-classes
		NumClasses = Multi[0]
		Class = [Multi[1], Multi[2], Multi[3]]
	elif Dual[0]: #only worry about the newer class
		#we only get the hp bonus if the old class is reactivated
		if (Levels[0]<=Levels[1]):
			return
		Class = [ClassTable.GetValue (Dual[2], 5)]
		#if Level and LevelDiff are passed, we assume it is correct
		if IsDualSwap(pc) and not Level and not LevelDiff:
			LevelDiffs = [LevelDiffs[1], LevelDiffs[0], LevelDiffs[2]]
	if NumClasses>len(Levels):
		return

	#get the correct hp for barbarians
	Kit = GetKitIndex (pc)
	ClassName = None
	if Kit and not Dual[0] and Multi[0]<2:
		KitName = KitListTable.GetValue (Kit, 0, 0)
		if ClassTable.GetRowIndex (KitName) >= 0:
			ClassName = KitName

	#loop through each class and update the hp
	OldHP = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS, 1)
	CurrentHP = 0
	Divisor = float (NumClasses)
	for i in range (NumClasses):
		#check this classes hp table for any gain
		if not ClassName or NumClasses > 1:
			ClassName = ClassTable.GetRowName (ClassTable.FindValue (5, Class[i]) )
		HPTable = ClassTable.GetValue (ClassName, "HP")
		HPTable = GemRB.LoadTableObject (HPTable)

		#make sure we are within table ranges
		MaxLevel = HPTable.GetRowCount()-1
		LowLevel = Levels[i]-LevelDiffs[i]
		HiLevel = Levels[i]
		if LowLevel >= HiLevel:
			continue
		if LowLevel < 0:
			LowLevel = 0
		elif LowLevel > MaxLevel:
			LowLevel = MaxLevel
		if HiLevel < 0:
			HiLevel = 0
		elif HiLevel > MaxLevel:
			HiLevel = MaxLevel

		#add all the hp for the given level
		#we use ceil to ensure each class gets hp
		for level in range(LowLevel, HiLevel):
			rolls = HPTable.GetValue (level, 1)
			bonus = HPTable.GetValue (level, 2)

			# we only do a roll if core diff or higher, or uncheck max
			if rolls:
				if GemRB.GetVar ("Difficulty Level") < 3:
					CurrentHP += ceil (GemRB.Roll (rolls, HPTable.GetValue (level, 0), bonus) / Divisor)
				else:
					CurrentHP += ceil ((rolls * HPTable.GetValue (level, 0) + bonus) / Divisor)
			else:
				CurrentHP += ceil (bonus / Divisor)
			CurrentHP = int (CurrentHP)

	#update our hp values
	GemRB.SetPlayerStat (pc, IE_MAXHITPOINTS, CurrentHP+OldHP)
	GemRB.SetPlayerStat (pc, IE_HITPOINTS, GemRB.GetPlayerStat (pc, IE_HITPOINTS, 1)+CurrentHP)
	return

def SetEncumbranceLabels (Window, Label, Label2, pc):
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	# encumbrance
	max_encumb = StrModTable.GetValue (sstr, 3) + StrModExTable.GetValue (ext_str, 3)
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Label = Window.GetControl (0x10000043)
	Label.SetText (str (encumbrance) + ":")

	Label2 = Window.GetControl (0x10000044)
	Label2.SetText (str (max_encumb) + ":")
	ratio = (0.0 + encumbrance) / max_encumb
	if ratio > 1.0:
		Label.SetTextColor (255, 0, 0)
		Label2.SetTextColor (255, 0, 0)
	elif ratio > 0.8:
		Label.SetTextColor (255, 255, 0)
		Label2.SetTextColor (255, 0, 0)
	else:
		Label.SetTextColor (255, 255, 255)
		Label2.SetTextColor (255, 0, 0)
	return

def GearsClicked():
	GemRB.GamePause(2,0)

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
	DiscWindow = Window = GemRB.LoadWindowObject (0)
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
		DiscWindow.SetVisible (1)

# returns an array: first field is 0 - not dual classed; 1 - kit/class; 2 - class/class
# the second and third field hold the kit/class index for each class
# if invoked with verbose==0 only returns 0 or 1 (is or is not dual classed)
def IsDualClassed(actor, verbose):
	Dual = GemRB.GetPlayerStat (actor, IE_MC_FLAGS)
	Dual = Dual & ~(MC_EXPORTABLE|MC_PLOT_CRITICAL|MC_BEENINPARTY|MC_HIDDEN)

	if verbose:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = ClassTable.FindValue (5, Class)
		Multi = ClassTable.GetValue (ClassIndex, 4)
		DualInfo = []
		KitIndex = GetKitIndex (actor)

		if (Dual & MC_WAS_ANY_CLASS) > 0: # first (previous) class of the dual class
			MCColumn = ClassTable.GetColumnIndex ("MC_WAS_ID")
			FirstClassIndex = ClassTable.FindValue (MCColumn, Dual & MC_WAS_ANY_CLASS)
			if KitIndex:
				DualInfo.append (1)
				DualInfo.append (KitIndex)
			else:
				DualInfo.append (2)
				DualInfo.append (FirstClassIndex)

			# use the first class of the multiclass bunch that isn't the same as the first class
			Mask = 1
			for i in range (1,16):
				if Multi & Mask:
					ClassIndex = ClassTable.FindValue (5, i)
					if ClassIndex == FirstClassIndex:
						Mask = 1 << i
						continue
					DualInfo.append (ClassIndex)
					break
				Mask = 1 << i
			return DualInfo
		else:
			return (0,-1,-1)
	else:
		if (Dual & MC_WAS_ANY_CLASS) > 0:
			return (1,-1,-1)
		else:
			return (0,-1,-1)

# Determines if the dualclassed actor has the two base classes in reverse order.
# This happens since dualclassed actors are treated as multiclassed and there
# are only so many valid multiclasses.
# Example: diviner/fighter maps to FIGHTER_MAGE instead of MAGE_FIGHTER
def IsDualSwap (actor):
	Dual = IsDualClassed (actor, 1)

	# not dual classed
	if Dual[0] == 0:
		return 0

	# split the full class name into its individual parts
	# i.e FIGHTER_MAGE becomes [FIGHTER, MAGE]
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Class = Class.split("_")

	# get our old class name
	if Dual[0] == 2:
		BaseClass = ClassTable.GetRowName (Dual[1])
	else:
		BaseClass = GetKitIndex (actor)
		BaseClass = KitListTable.GetValue (BaseClass, 7)
		BaseClass = ClassTable.FindValue (5, BaseClass)
		BaseClass = ClassTable.GetRowName (BaseClass)

	# if our old class is the first class, we need to swap
	if Class[0] == BaseClass:
		return 1

	return 0

##
#
# Determines if an actor is multiclassed and returns the individual classes.
# @param actor The character reference.
# @param verbose If 0 simply returns 0 if not multi.
# @return A tuple - (IsMultiClassed, Class1, Class2, Class3).
def IsMultiClassed (actor, verbose):
	# get our base class
	ClassIndex = ClassTable.FindValue (5, GemRB.GetPlayerStat (actor, IE_CLASS))
	IsMulti = ClassTable.GetValue (ClassIndex, 4) # 0 if not multi'd
	IsDual = IsDualClassed (actor, 0)

	# dual-class char's look like multi-class chars
	if (IsMulti == 0) or (IsDual[0] > 0):
		return (0,-1,-1,-1)
	elif verbose == 0:
		return (IsMulti,-1,-1,-1)

	# get all our classes (leave space for our number of classes in the return array)
	Classes = [0]*3
	NumClasses = 0
	Mask = 1 # we're looking at multiples of 2
	ClassNames = ClassTable.GetRowName(ClassIndex).split("_")

	# loop through each class and test it as a mask
	# TODO: make 16 dynamic? -- allows for custom classes (not just kits)
	for i in range (1, 16):
		if IsMulti&Mask: # it's part of this class
			#we need to place the classes in the array based on their order in the name,
			#NOT the order they are detected in
			CurrentName = ClassTable.GetRowName (ClassTable.FindValue (5, i));
			for j in range(len(ClassNames)):
				if ClassNames[j] == CurrentName:
					Classes[j] = i # mask is (i-1)^2 where i is class id
			NumClasses = NumClasses+1
		Mask = 1 << i # shift to the next multiple of 2 for testing

	# in case we couldn't figure out to which classes the multi belonged
	if NumClasses < 2:
		return (0,-1,-1,-1)

	# return the tuple
	return (NumClasses, Classes[0], Classes[1], Classes[2])

def GetNextLevelExp (Level, Class):
	Row = NextLevelTable.GetRowIndex (Class)
	if Level < NextLevelTable.GetColumnCount (Row):
		return str (NextLevelTable.GetValue (Row, Level) )

	return 0

# returns boolean - true if can level
def CanLevelUp(actor):
	# get our class and placements for Multi'd and Dual'd characters
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Multi = IsMultiClassed (actor, 1)
	Dual = IsDualClassed (actor, 1)

	# get all the levels and overall xp here
	Levels = [GemRB.GetPlayerStat (actor, IE_LEVEL), GemRB.GetPlayerStat (actor, IE_LEVEL2), GemRB.GetPlayerStat (actor, IE_LEVEL3)]
	xp = GemRB.GetPlayerStat (actor, IE_XP)

	if Multi[0] > 1: # multiclassed
		xp = xp/Multi[0] # divide the xp evenly between the classes
		for i in range (Multi[0]):
			# if any class can level, return 1
			ClassIndex = ClassTable.FindValue (5, Multi[i+1])
			if int(GetNextLevelExp (Levels[i], ClassTable.GetRowName (ClassIndex))) <= xp:
				return 1

		# didn't find a class that could level
		return 0
	elif Dual[0] > 0: # dual classed
		if IsDualSwap (actor): # swap the levels if we have to
			Levels = [Levels[1], Levels[0], Levels[2]]
		# get the class we can level
		Class = ClassTable.GetRowName (Dual[2])

	# check the class that can be level (single or dual)
	return int(GetNextLevelExp (Levels[0], Class)) <= xp

# learn all the priest spells up to a given spell level
# mask is 0x4000 = cleric and 0x8000 = druid
# level 1 is 1st level spells
def LearnPriestSpells (pc, level, mask):
	if level > 7: # make sure we don't have too high a level
		level = 7

	# go through each level
	alignment = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)
	for i in range (level):
		learnable = GetLearnablePriestSpells (mask, alignment, i+1)

		for spell in learnable:
			# if the spell isn't learned, learn it
			if HasSpell (pc, IE_SPELL_TYPE_PRIEST, i, spell) < 0:
				GemRB.LearnSpell (pc, spell)

# removes all known spells of a type between two inclusive levels
# type should be IE_SPELL_TYPE_*
# if noslots is true, then we also set memorizable counts to 0
# kit is only used to distinguish priest spells for ranger/cleric duals
def RemoveKnownSpells (pc, type, level1=1, level2=1, noslots=0, kit=0):
	# choose the correct limit based upon class type
	if type == IE_SPELL_TYPE_WIZARD:
		limit = 9
	elif type == IE_SPELL_TYPE_PRIEST:
		limit = 7

		# make sure that we get the original kit, if we have one
		if kit:
			originalkit = GetKitIndex (pc)

			if originalkit: # kitted; find the class value
				originalkit = KitListTable.GetValue (originalkit, 7)
			else: # just get the class value
				originalkit = GemRB.GetPlayerStat (pc, IE_CLASS)

			# this is is specifically for dual-classes and will not work to remove only one
			# spell type from a ranger/cleric multi-class
			if ClassSkillsTable.GetValue (originalkit, 0, 0) != "*": # knows druid spells
				originalkit = 0x8000
			elif ClassSkillsTable.GetValue (originalkit, 1, 0) != "*": # knows cleric spells
				originalkit = 0x4000
			else: # don't know any other spells
				originalkit = 0

			# don't know how this would happen, but better to be safe
			if originalkit == kit:
				originalkit = 0
	elif type == IE_SPELL_TYPE_INNATE:
		limit = 1
	else: # can't do anything if an improper spell type is sent
		return 0

	# make sure we're within parameters
	if level1 < 1 or level2 > limit or level1 > level2:
		return 0

	# remove all spells for each level
	for level in range (level1-1, level2):
		# we need the count because we remove each spell in reverse order
		count = GemRB.GetKnownSpellsCount (pc, type, level)
		mod = count-1

		for spell in range (count):
			# see if we need to check for kit
			if type == IE_SPELL_TYPE_PRIEST and kit:
				# get the spells ref data
				ref = GemRB.GetKnownSpell (pc, type, level, mod-spell)
				ref = GemRB.GetSpell (ref['SpellResRef'], 1)

				# we have to look at the originalkit as well specifically for ranger/cleric dual-classes
				# we wouldn't want to remove all cleric spells and druid spells if we lost our cleric class
				# only the cleric ones
				if kit&ref['SpellDivine'] or (originalkit and not originalkit&ref['SpellDivine']):
					continue

			# remove the spell
			GemRB.RemoveSpell (pc, type, level, mod-spell)

		# remove memorization counts if disired
		if noslots:
			GemRB.SetMemorizableSpellsCount (pc, 0, type, level)

	# return success
	return 1

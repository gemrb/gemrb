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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$


# GUICommonWindows.py - functions to open common windows in lower part of the screen

import GemRB
from GUIDefines import *
from ie_stats import *
from GUIClasses import GWindow
from LUCommon import CanLevelUp
from GUICommon import *
import GUICommon

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

TimeWindow = None
PortWindow = None
MenuWindow = None
MainWindow = None
global PortraitWindow
PortraitWindow = None
ActionsWindow = None
OptionsWindow = None
DraggedPortrait = None
DiscWindow = None

# Buttons:
# 0 CNTREACH
# 1 INVNT
# 2 MAP
# 3 MAGE
# 4 AI
# 5 STATS
# 6 JRNL
# 7 PRIEST
# 8 OPTION
# 9 REST
# 10 TXTE

def OpenCommonWindows ():
	global TimeWindow, PortWindow, MenuWindow, MainWindow

	TimeWindow = GemRB.LoadWindowObject (0)
	PortWindow = GemRB.LoadWindowObject (1)
	MenuWindow = GemRB.LoadWindowObject (2)
	MainWindow = GemRB.LoadWindowObject (3)

	Window = MenuWindow

	# Can't Reach ???
	Button = Window.GetControl (0)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CntReachPress")

	# AI
	Button = Window.GetControl (4)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "AIPress")

	# Message popup
	Button = Window.GetControl (10)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "TxtePress")


	SetupMenuWindowControls (Window)


	TimeWindow.SetVisible (1)
	PortWindow.SetVisible (1)
	MenuWindow.SetVisible (1)
	MainWindow.SetVisible (1)
	
def CloseCommonWindows ():
	global MainWindow

	if MainWindow == None:
		return
	#if TimeWindow == None:
	#	return
	
	if MainWindow:
		MainWindow.Unload ()
	if TimeWindow:
		TimeWindow.Unload ()
	if PortWindow:
		PortWindow.Unload ()
	if MenuWindow:
		MenuWindow.Unload ()

	MainWindow = None

def SetupMenuWindowControls (Window):
	# Inventory
	Button = Window.GetControl (1)
	Button.SetTooltip (41601)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Map
	Button = Window.GetControl (2)
	Button.SetTooltip (41625)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Mage
	Button = Window.GetControl (3)
	Button.SetTooltip (41624)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMageWindow")
	# Stats
	Button = Window.GetControl (5)
	Button.SetTooltip (4707)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Journal
	Button = Window.GetControl (6)
	Button.SetTooltip (41623)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	# Priest
	Button = Window.GetControl (7)
	Button.SetTooltip (4709)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenPriestWindow")

	# Options
	Button = Window.GetControl (8)
	Button.SetTooltip (41626)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Rest
	Button = Window.GetControl (9)
	Button.SetTooltip (41628)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "RestPress")


	# AI
	Button = Window.GetControl (4)
	Button.SetTooltip (41631) # or 41646 Activate ...
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenFloatMenuWindow")

	# (Un)Lock view on character
	Button = Window.GetControl (0)
	Button.SetTooltip (41647)  # or 41648 Unlock ...
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnLockViewPress")

	# Message popup
	Button = Window.GetControl (10)
	Button.SetTooltip (41660)  # or 41661 Close ...


def OnLockViewPress ():
	GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR | SF_ALWAYSCENTER, OP_OR)
	print "OnLockViewPress"

def AIPress ():
	print "AIPress"

def TxtePress ():
	print "TxtePress"

def SetupActionsWindowControls (Window):
	# time button
	Button = Window.GetControl (0)
	Button.SetAnimation ("WMTIME")
	Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)

	# 41627 - Return to the Game World
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetTooltip (41627)

	# Select all characters
	Button = Window.GetControl (1)
	Button.SetTooltip (41659)

	# Abort current action
	Button = Window.GetControl (3)
	Button.SetTooltip (41655)

	# Formations
	Button = Window.GetControl (4)
	Button.SetTooltip (44945)
	


# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	#return GemRB.GetPlayerPortrait( actor, which)

	# only the lowest byte is meaningful here (OneByteAnimID)
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID) & 255
	row = "0x%02X" %anim_id

	return GUICommon.AppearanceAvatarTable.GetValue (row, which)
	

def UpdateAnimation ():
	pc = GemRB.GameGetSelectedPCSingle ()

	disguise = GemRB.GetGameVar ("APPEARANCE")
	if disguise == 2: #dustman
		animid = "DR"				
	elif disguise == 1: #zombie
		animid = "ZO"
	else:
		slot = GemRB.GetEquippedQuickSlot (pc)
		item = GemRB.GetSlotItem (pc, slot )
		animid = ""
		if item:
			item = GemRB.GetItem(item["ItemResRef"])
			if item:
				animid = item["AnimationType"]

	BioTable = GemRB.LoadTableObject ("BIOS")
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = BioTable.GetValue (Specific, "PC")
	AnimTable = GemRB.LoadTableObject ("ANIMS")
	if animid=="":
		animid="*"
	value = AnimTable.GetValue (animid, AvatarName)
	if value<0:
		return
	GemRB.SetPlayerStat (pc, IE_ANIMATION_ID, value)
	return


SelectionChangeHandler = None
SelectionChangeMultiHandler = None

def SetSelectionChangeHandler (handler):
	global SelectionChangeHandler

	# Switching from walking to non-walking environment:
	#   set the first selected PC in walking env as a selected
	#   in nonwalking env
	if (not SelectionChangeHandler) and handler:
		sel = GemRB.GameGetFirstSelectedPC ()
		if not sel:
			sel = 1
		GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	# redraw selection on change main selection | single selection
	SelectionChanged ()

def SetSelectionChangeMultiHandler (handler):
	global SelectionChangeMultiHandler
	SelectionChangeMultiHandler = handler
	#SelectionChanged ()

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()

portrait_hp_numeric = [0, 0, 0, 0, 0, 0]


def OpenPortraitWindow (needcontrols):
	global PortraitWindow

	PortraitWindow = Window = GemRB.LoadWindowObject (1)

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ('PressedPortrait', i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, "PortraitButtonOnDrag")
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "PortraitButtonOnMouseEnter")
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, "PortraitButtonOnMouseLeave")

		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

		ButtonHP = Window.GetControl (6 + i)
		ButtonHP.SetVarAssoc ('PressedPortraitHP', i)
		ButtonHP.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PortraitButtonHPOnPress")

		portrait_hp_numeric[i] = 0

	UpdatePortraitWindow ()
	SelectionChanged()
	return Window

def UpdatePortraitWindow ():
	Window = PortraitWindow

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		ButtonHP = Window.GetControl (6 + i)

		pic = GemRB.GetPlayerPortrait (i+1, 0)
		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			ButtonHP.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue
		
		#sel = GemRB.GameGetSelectedPCSingle () == i + 1
		Button.SetBAM (pic, 0, 0, -1)

		state = GemRB.GetPlayerStat (i+1, IE_STATE_ID)
		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)

		if state & STATE_DEAD:
				cycle = 9
		elif state & STATE_HELPLESS:
				cycle = 8
		elif state & STATE_PETRIFIED:
				cycle = 7
		elif state & STATE_PANIC:
				cycle = 6
		elif state & STATE_POISONED:
				cycle = 2
		elif hp<hp_max/5:
			cycle = 4
		else:
			cycle = 0

		if cycle<6:
			Button.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED | IE_GUI_BUTTON_PLAYRANDOM|IE_GUI_BUTTON_DRAGGABLE, OP_SET)
		else:
			Button.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED | IE_GUI_BUTTON_DRAGGABLE, OP_SET)
		Button.SetAnimation (pic, cycle)
		

		ButtonHP.SetFlags(IE_GUI_BUTTON_PICTURE, OP_SET)

		if hp_max<1:
			ratio = 0.0
		else:
			ratio = (hp + 0.0) / hp_max
			if ratio > 1.0: ratio = 1.0
		r = int (255 * (1.0 - ratio))
		g = int (255 * ratio)

		ButtonHP.SetText ("%d / %d" %(hp, hp_max))
		ButtonHP.SetTextColor (r, g, 0, False)
		ButtonHP.SetBAM ('FILLBAR', 0, 0, -1)
		ButtonHP.SetPictureClipping (ratio)

		if portrait_hp_numeric[i]:
			op = OP_NAND
		else:
			op = OP_OR
			
		ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)


		#if sel:
		#	Button.EnableBorder(FRAME_PC_SELECTED, 1)
		#else:
		#	Button.EnableBorder(FRAME_PC_SELECTED, 0)
	return

def PortraitButtonOnDrag ():
	global DraggedPortrait

	#they start from 1
	DraggedPortrait = GemRB.GetVar ("PressedPortrait")+1
	GemRB.DragItem (DraggedPortrait, -1, "")
	return

def PortraitButtonOnPress ():
	i = GemRB.GetVar ('PressedPortrait')

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

def PortraitButtonHPOnPress ():
	Window = PortraitWindow
	
	i = GemRB.GetVar ('PressedPortraitHP')

	portrait_hp_numeric[i] = not portrait_hp_numeric[i]
	ButtonHP = Window.GetControl (6 + i)

	if portrait_hp_numeric[i]:
		op = OP_NAND
	else:
		op = OP_OR
			
	ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)
	return

def StopAllOnPress ():
	for i in range (PARTY_SIZE):
		if GemRB.GameIsPCSelected(i + 1):
			GemRB.ClearActions(i + 1)
	return

# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
		if SelectionChangeMultiHandler:
			SelectionChangeMultiHandler ()
	else:
		sel = GemRB.GameGetSelectedPCSingle ()
		for i in range (0, PARTY_SIZE):
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

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i)
		Button.EnableBorder (FRAME_PC_TARGET, 1)

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ('PressedPortrait')
	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i)
		Button.EnableBorder (FRAME_PC_TARGET, 0)
	return

def DisableAnimatedWindows ():
	global ActionsWindow, OptionsWindow
	GemRB.SetVar ("PortraitWindow", -1)
	ActionsWindow = GWindow( GemRB.GetVar ("ActionsWindow") )
	GemRB.SetVar ("ActionsWindow", -1)
	OptionsWindow = GWindow( GemRB.GetVar ("OptionsWindow") )
	GemRB.SetVar ("OptionsWindow", -1)
	GemRB.GamePause (1,1)

def EnableAnimatedWindows ():
	GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
	GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
	GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
	GemRB.GamePause (0,1)


def SetEncumbranceButton (Window, ButtonID, pc):
	"""Set current/maximum encumbrance button for a given pc,
	using numeric font"""
	
	# Loading tables of modifications
	Table = GemRB.LoadTableObject ("strmod")
	TableEx = GemRB.LoadTableObject ("strmodex")
	
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	# Compute encumbrance
	maximum = Table.GetValue (sstr, 3) + TableEx.GetValue (ext_str, 3)
	current = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Button = Window.GetControl (ButtonID)
	# FIXME: there should be a space before LB symbol (':')
	Button.SetText (str (current) + ":\n\n\n\n" + str (maximum) + ":")

	# Set button color for overload
	ratio = (0.0 + current) / maximum
	if ratio > 1.0:
		Button.SetTextColor (255, 0, 0, True)
	elif ratio > 0.8:
		Button.SetTextColor (255, 255, 0, True)
	else:
		Button.SetTextColor (255, 255, 255, True)

	# FIXME: Current encumbrance is hardcoded
	# Unloading tables is not necessary, i think (they will stay cached)


def SetItemButton (Window, Button, Slot, PressHandler, RightPressHandler):
	if Slot != None:
		Item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED
		#Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		#Button.SetSprites ('IVSLOT', 0,  0, 0, 0, 0)
		Button.SetItemIcon (Slot['ItemResRef'],0)

		if Item['StackAmount'] > 1:
			Button.SetText (str (Slot['Usages0']))
		else:
			Button.SetText ('')


		if not identified or Item['ItemNameIdentified'] == -1:
			Button.SetTooltip (Item['ItemName'])
		else:
			Button.SetTooltip (Item['ItemNameIdentified'])

		#Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		#Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PressHandler)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, RightPressHandler)
		#Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, ShiftPressHandler)
		#Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, DragDropHandler)

	else:
		#Button.SetVarAssoc ("LeftIndex", -1)
		Button.SetItemIcon ('')
		Button.SetTooltip (4273)  # Ground Item
		Button.SetText ('')
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "")
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		#Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, "")
		#Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, "")

def OpenWaitForDiscWindow ():
	global DiscWindow
	#print "OpenWaitForDiscWindow"

	if DiscWindow:
		GemRB.HideGUI ()
		if DiscWindow:
			DiscWindow.Unload ()
		GemRB.SetVar ("OtherWindow", -1)
		# ...LoadWindowPack()
		GemRB.EnableAnimatedWindows ()
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
	# 31568 - Please  place the PS:T DVD 
	# 31569 - in drive
	# 31570 - Wrong disc in drive
	# 31571 - There is no disc in drive
	# 31578 - No disc could be found in drive. Please place Disc 1 in drive.
	# 49152 - To quit the game, press Alt-F4


	try:
		GemRB.UnhideGUI ()
	except:
		DiscWindow.SetVisible (1)

def CheckLevelUp(pc):
	GemRB.SetVar ("CheckLevelUp"+str(pc), CanLevelUp (pc))

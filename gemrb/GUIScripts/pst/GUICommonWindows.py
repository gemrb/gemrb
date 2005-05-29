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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUICommonWindows.py,v 1.32 2005/05/29 22:52:47 edheldil Exp $


# GUICommonWindows.py - functions to open common windows in lower part of the screen

import GemRB
from GUIDefines import *
from ie_stats import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

TimeWindow = None
PortWindow = None
MenuWindow = None
MainWindow = None
global PortraitWindow
PortraitWindow = None
ActionsWindow = None

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

	TimeWindow = GemRB.LoadWindow (0)
	PortWindow = GemRB.LoadWindow (1)
	MenuWindow = GemRB.LoadWindow (2)
	MainWindow = GemRB.LoadWindow (3)

	Window = MenuWindow

	# Can't Reach ???
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CntReachPress")

	# AI
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "AIPress")

	# Message popup
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "TxtePress")


	SetupMenuWindowControls (Window)


	GemRB.SetVisible (TimeWindow, 1)
	GemRB.SetVisible (PortWindow, 1)
	GemRB.SetVisible (MenuWindow, 1)
	GemRB.SetVisible (MainWindow, 1)
	
def CloseCommonWindows ():
	global MainWindow

	if MainWindow == None:
		return
	#if TimeWindow == None:
	#	return
	
	GemRB.UnloadWindow (MainWindow)
	GemRB.UnloadWindow (TimeWindow)
	GemRB.UnloadWindow (PortWindow)
	GemRB.UnloadWindow (MenuWindow)

	MainWindow = None

def SetupMenuWindowControls (Window):
	# Inventory
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetTooltip (Window, Button, 41601)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Map
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetTooltip (Window, Button, 41625)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Mage
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 41624)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageWindow")
	# Stats
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetTooltip (Window, Button, 4707)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Journal
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetTooltip (Window, Button, 41623)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")

	# Priest
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetTooltip (Window, Button, 4709)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestWindow")

	# Options
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetTooltip (Window, Button, 41626)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Rest (temporary store window for debug purpose)
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetTooltip (Window, Button, 41628)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")


	# AI
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 41631) # or 41646 Activate ...
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFloatMenuWindow")

	# Can't Reach ???
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetTooltip (Window, Button, 41647)  # or 41648 Unlock ...
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPartyManageWindow")
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenContainerWindow")
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CntReachPress")

	# Message popup
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetTooltip (Window, Button, 41660)  # or 41661 Close ...


def CntReachPress ():
	print "CntReachPress"

def AIPress ():
	print "AIPress"

def TxtePress ():
	print "TxtePress"

def RestPress ():
	print "RestPress"

def SetupActionsWindowControls (Window):
	# time button
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetAnimation (Window, Button, "WMTIME")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)

	# 41627 - Return to the Game World
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetTooltip (Window, Button, 41627)

	# Select all characters
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetTooltip (Window, Button, 41659)

	# Abort current action
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 41655)

	# Formations
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 44945)
	


# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	#return GemRB.GetPlayerPortrait( actor, which)

	PortraitTable = GemRB.LoadTable ("PDOLLS")
	# only the lowest byte is meaningful here (OneByteAnimID)
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID) & 255
	row = "0x%02X" %anim_id

	return GemRB.GetTableValue (PortraitTable, row, which)
	


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

	PortraitWindow = Window = GemRB.LoadWindow (1)

	for i in range (PARTY_SIZE):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, 'PressedPortrait', i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")

		GemRB.SetButtonBorder (Window, Button, FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		GemRB.SetButtonBorder (Window, Button, FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

		ButtonHP = GemRB.GetControl (Window, 6 + i)
		GemRB.SetVarAssoc (Window, ButtonHP, 'PressedPortraitHP', i)
		GemRB.SetEvent (Window, ButtonHP, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonHPOnPress")

		portrait_hp_numeric[i] = 0

	UpdatePortraitWindow ()
	return Window

def UpdatePortraitWindow ():
	Window = PortraitWindow

	for i in range (PARTY_SIZE):
		Button = GemRB.GetControl (Window, i)
		ButtonHP = GemRB.GetControl (Window, 6 + i)

		pic = GemRB.GetPlayerPortrait (i+1, 0)
		if not pic:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			GemRB.SetButtonFlags (Window, ButtonHP, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue
		
		#sel = GemRB.GameIsPCSelected (i+1)
		sel = GemRB.GameGetSelectedPCSingle () == i + 1
		GemRB.SetButtonBAM (Window, Button, pic, 0, 0, -1)
		GemRB.SetAnimation (Window, Button, pic)
		
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED | IE_GUI_BUTTON_PLAYRANDOM, OP_SET)
		#GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_ANIMATED, OP_SET)

		GemRB.SetButtonFlags(Window, ButtonHP, IE_GUI_BUTTON_PICTURE, OP_SET)

		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)

		ratio = (hp + 0.0) / hp_max
		if ratio > 1.0: ratio = 1.0
		r = int (255 * (1.0 - ratio))
		g = int (255 * ratio)

		GemRB.SetText (Window, ButtonHP, "%d / %d" %(hp, hp_max))
		GemRB.SetButtonTextColor (Window, ButtonHP, r, g, 0, False)
		GemRB.SetButtonBAM (Window, ButtonHP, 'FILLBAR', 0, 0, -1)
		GemRB.SetButtonPictureClipping (Window, ButtonHP, ratio)

		if portrait_hp_numeric[i]:
			op = OP_NAND
		else:
			op = OP_OR
			
		GemRB.SetButtonFlags (Window, ButtonHP, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)


		if sel:
			#GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_SELECTED)
			GemRB.EnableButtonBorder(Window, Button, FRAME_PC_SELECTED, 1)
		else:
			#GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_UNPRESSED)
			GemRB.EnableButtonBorder(Window, Button, FRAME_PC_SELECTED, 0)


def PortraitButtonOnPress ():
	i = GemRB.GetVar ('PressedPortrait')

	if (not SelectionChangeHandler):
		GemRB.GameSelectPC (i + 1, True, SELECT_REPLACE)
	else:
		GemRB.GameSelectPCSingle (i + 1)
		SelectionChanged ()
		RunSelectionChangeHandler ()


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


def PortraitButtonHPOnPress ():
	Window = PortraitWindow
	
	i = GemRB.GetVar ('PressedPortraitHP')

	portrait_hp_numeric[i] = not portrait_hp_numeric[i]
	ButtonHP = GemRB.GetControl (Window, 6 + i)

	if portrait_hp_numeric[i]:
		op = OP_NAND
	else:
		op = OP_OR
			
	GemRB.SetButtonFlags (Window, ButtonHP, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)



def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)

# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		for i in range (0, PARTY_SIZE):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
		if SelectionChangeMultiHandler:
			SelectionChangeMultiHandler ()
	else:
		sel = GemRB.GameGetSelectedPCSingle ()
		for i in range (0, PARTY_SIZE):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, i + 1 == sel)

def PortraitButtonOnDragDrop ():
	i = GemRB.GetVar ('PressedPortrait')
	print "DragDrop"

def PortraitButtonOnMouseOver ():
	i = GemRB.GetVar ('PressedPortrait')
	if GemRB.IsDraggingItem ():
		Button = GemRB.GetControl (PortraitWindow, i)
		GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_TARGET, 1)

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ('PressedPortrait')
	if GemRB.IsDraggingItem ():
		Button = GemRB.GetControl (PortraitWindow, i)
		GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_TARGET, 0)


def DisableAnimatedWindows ():
	global ActionsWindow
	#PortraitWindow = GemRB.GetVar ("PortraitWindow")
	GemRB.SetVar ("PortraitWindow", -1)
	ActionsWindow = GemRB.GetVar ("ActionsWindow")
	GemRB.SetVar ("ActionsWindow", -1)

def EnableAnimatedWindows ():
	GemRB.SetVar ("PortraitWindow", PortraitWindow)
	GemRB.SetVar ("ActionsWindow", ActionsWindow)


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
	Class = GemRB.FindTableValue (ClassTable, 5, Class)
	Multi = GemRB.GetTableValue (ClassTable, Class, 4)
	if Multi:
		if Class == 7:
			#fighter/mage
			Class = GemRB.FindTableValue (ClassTable, 5, 1)
		else:
			#fighter/thief
			Class = GemRB.FindTableValue (ClassTable, 5, 4)
		SaveName2 = GemRB.GetTableValue (ClassTable, Class, 3)
		Class = 0  #fighter
		print "SaveName2", SaveName2

	SaveName1 = GemRB.GetTableValue (ClassTable, Class, 3)
	print "SaveName1", SaveName1
	
	for row in range(5):
		tmp1 = GetSavingThrow (SaveName1, row, level1)
		if Multi:
			tmp2 = GetSavingThrow (SaveName2, row, level2)
			if tmp2<tmp1:
				tmp1=tmp2
		GemRB.SetPlayerStat (pc, IE_SAVEVSDEATH+row, tmp1)
		print "Savingthrow:", tmp1
	return


def SetEncumbranceButton (Window, ButtonID, pc):
	"""Set current/maximum encumbrance button for a given pc,
	using numeric font"""
	
	# Loading tables of modifications
	Table = GemRB.LoadTable ("strmod")
	TableEx = GemRB.LoadTable ("strmodex")
	
	# Getting the character's strength
	sstr = GemRB.GetPlayerStat (pc, IE_STR)
	ext_str = GemRB.GetPlayerStat (pc, IE_STREXTRA)

	# Compute encumbrance
	maximum = GemRB.GetTableValue (Table, sstr, 3) + GemRB.GetTableValue (TableEx, ext_str, 3)
	current = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)

	Button = GemRB.GetControl (Window, ButtonID)
	# FIXME: there should be a space before LB symbol (':')
	GemRB.SetText (Window, Button, str (current) + ":\n\n\n\n" + str (maximum) + ":")

	# Set button color for overload
	ratio = (0.0 + current) / maximum
	if ratio > 1.0:
		GemRB.SetButtonTextColor (Window, Button, 255, 0, 0, True)
	elif ratio > 0.8:
		GemRB.SetButtonTextColor (Window, Button, 255, 255, 0, True)
	else:
		GemRB.SetButtonTextColor (Window, Button, 255, 255, 255, True)

	# FIXME: Current encumbrance is hardcoded
	# Unloading tables is not necessary, i think (they will stay cached)
	#GemRB.UnloadTable (Table)
	#GemRB.UnloadTable (TableEx)


def SetItemButton (Window, Button, Slot, PressHandler, RightPressHandler):
	if Slot != None:
		Item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED
		#GemRB.SetVarAssoc (Window, Button, "LeftIndex", LeftTopIndex+i)
		#GemRB.SetButtonSprites (Window, Button, 'IVSLOT', 0,  0, 0, 0, 0)
		GemRB.SetItemIcon (Window, Button, Slot['ItemResRef'],0)

		if Item['StackAmount'] > 1:
			GemRB.SetText (Window, Button, str (Slot['Usages0']))
		else:
			GemRB.SetText (Window, Button, '')


		if not identified or Item['ItemNameIdentified'] == -1:
			GemRB.SetTooltip (Window, Button, Item['ItemName'])
		else:
			GemRB.SetTooltip (Window, Button, Item['ItemNameIdentified'])

		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_OR)
		#GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, PressHandler)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, RightPressHandler)
		#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, ShiftPressHandler)
		#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, DragDropHandler)

	else:
		#GemRB.SetVarAssoc (Window, Button, "LeftIndex", -1)
		GemRB.SetItemIcon (Window, Button, '')
		GemRB.SetTooltip (Window, Button, 4273)  # Ground Item
		GemRB.SetText (Window, Button, '')
		GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_PICTURE, OP_NAND)

		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_RIGHT_PRESS, "")
		#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "")
		#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "")

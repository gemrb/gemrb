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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/GUICommonWindows.py,v 1.3 2004/10/15 20:38:55 avenger_teambg Exp $


# GUICommonWindows.py - functions to open common windows in lower part of the screen

###################################################

import GemRB
from GUIDefines import *
from ie_stats import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

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

def SetupMenuWindowControls (Window):
	# FIXME: add "(key)" to tooltips!

	# Return to Game
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetTooltip (Window, Button, 16313)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "ReturnToGame")

	# Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetTooltip (Window, Button, 16310)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	# Journal
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetTooltip (Window, Button, 16308)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenJournalWindow")
	# Inventory
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 16307)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenInventoryWindow")

	# Records
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 16306)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 4)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenRecordsWindow")

	# Mage
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetTooltip (Window, Button, 16309)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 5)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMageWindow")
	# Priest
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetTooltip (Window, Button, 14930)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 6)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenPriestWindow")

	# Options
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetTooltip (Window, Button, 16311)
	GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetVarAssoc(Window, Button, "SelectedWindow", 7)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# Party mgmt
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetTooltip (Window, Button, 16312)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")

## 	# Rest
## 	Button = GemRB.GetControl (Window, 8)
## 	GemRB.SetTooltip (Window, Button, 11942)
## 	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")

	# AI
	#Button = GemRB.GetControl (Window, 9)
	#GemRB.SetTooltip (Window, Button, 41631) # or 41646 Activate ...
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFloatMenuWindow")
	return

def ReturnToGame ():
	print "ReturnToGame"
	CloseOtherWindow (None)

def AIPress ():
	print "AIPress"


def RestPress ():
	print "RestPress"


def SetupActionsWindowControls (Window):
	# 41627 - Return to the Game World

	# Select all characters
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetTooltip (Window, Button, 41659)

	# Abort current action
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetTooltip (Window, Button, 41655)

	# Formations
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 44945)


def GetActorClassTitle (actor):
	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)
	KitIndex = GemRB.GetPlayerStat (actor, IE_KIT) & 0xfff
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	ClassTable = GemRB.LoadTable ("classes")
	Class = GemRB.FindTableValue( ClassTable, 5, Class )
	KitTable = GemRB.LoadTable ("kitlist")

	if ClassTitle==0:
		if KitIndex == 0:
			ClassTitle=GemRB.GetTableValue(ClassTable, Class, 2)
		else:
			ClassTitle=GemRB.GetTableValue(KitTable, KitIndex, 2)

	GemRB.UnloadTable (ClassTable)
	GemRB.UnloadTable (KitTable)
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

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()

def PopulatePortraitWindow (Window):
	global PortraitWindow
	PortraitWindow = Window

	for i in range (6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_SHIFT_PRESS, "PortraitButtonOnShiftPress")
		#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "PortraitButtonOnDragDrop")
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_DRAG_DROP, "OnDropItemToPC")
		#GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_ENTER_BUTTON, "PortraitButtonOnMouseOver")
		#GemRB.SetEvent (Window, Button, IE_GUI_MOUSE_LEAVE_BUTTON, "PortraitButtonOnMouseLeave")

		GemRB.SetButtonBorder (Window, Button, FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		GemRB.SetButtonBorder (Window, Button, FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)
		pic = GemRB.GetPlayerPortrait (i+1, 1)
		if not pic:
			GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue
		
		#sel = GemRB.GameIsPCSelected (i+1)
		sel = GemRB.GameGetSelectedPCSingle () == i + 1
		#GemRB.SetButtonBAM (Window, Button, pic, 0, 0, 0)
		GemRB.SetButtonPicture(Window, Button, pic)
		
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ALIGN_TOP | IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		GemRB.SetButtonFont (Window, Button, 'NUMFONT')

		GemRB.SetVarAssoc (Window, Button, 'PressedPortrait', i)

		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)

		GemRB.SetText (Window, Button, "%d/%d" %(hp, hp_max))
		GemRB.SetTooltip (Window, Button, GemRB.GetPlayerName (i+1, 1) + "\n%d/%d" %(hp, hp_max))
		if sel:
			#GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_SELECTED)
			GemRB.EnableButtonBorder(Window, Button, FRAME_PC_SELECTED, 1)
		else:
			#GemRB.SetButtonState(Window, Button, IE_GUI_BUTTON_UNPRESSED)
			GemRB.EnableButtonBorder(Window, Button, FRAME_PC_SELECTED, 0)


def PortraitButtonOnPress ():
	i = GemRB.GetVar ("PressedPortrait")

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


def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)


# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		for i in range (6):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()
		for i in range (6):
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
	Multi = GemRB.GetTableValue (ClassTable, 4, Class)
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


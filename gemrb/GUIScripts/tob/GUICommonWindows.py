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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUICommonWindows.py,v 1.5 2004/09/17 22:25:01 avenger_teambg Exp $


# GUICommonWindows.py - functions to open common windows in lower part of the screen

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

	# Rest
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetTooltip (Window, Button, 11942)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")

	# AI
	#Button = GemRB.GetControl (Window, 9)
	#GemRB.SetTooltip (Window, Button, 41631) # or 41646 Activate ...
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFloatMenuWindow")
	return

def AIPress ():
	print "AIPress"
	return

def RestPress ():
	print "RestPress"
	return

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
	return

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
	# AI
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "AIPress")

	#Select All
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetTooltip (Window, Button, 10485)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")
	for i in range (0,6):
		Button = GemRB.GetControl (Window, i)
		GemRB.SetVarAssoc (Window, Button, "SelectedSingle", i)
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "PortraitButtonOnPress")

		pic = GemRB.GetPlayerPortrait (i+1,1)
		GemRB.SetButtonPicture(Window, Button, pic)
		GemRB.SetButtonFlags(Window, Button, IE_GUI_BUTTON_PICTURE, OP_SET)

		GemRB.SetButtonBorder (Window, Button, FRAME_PC_SELECTED, 1, 1,
2, 2, 0, 255, 0, 255)
		GemRB.SetButtonBorder (Window, Button, FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)
		GemRB.SetVarAssoc (Window, Button, "PressedPortrait", i)
	return

def PortraitButtonOnPress ():
	i = GemRB.GetVar ("PressedPortrait")

	if (not SelectionChangeHandler):
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

def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)
	return

# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		for i in range (0, 6):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()
		for i in range (0, 6):
			Button = GemRB.GetControl (PortraitWindow, i)

		for i in range (0, 6):
			Button = GemRB.GetControl (PortraitWindow, i)
			GemRB.EnableButtonBorder (PortraitWindow, Button, FRAME_PC_SELECTED, i + 1 == sel)


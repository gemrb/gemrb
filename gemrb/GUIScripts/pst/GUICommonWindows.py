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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUICommonWindows.py,v 1.13 2004/07/25 00:00:02 edheldil Exp $


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

	# Rest
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetTooltip (Window, Button, 41628)
	GemRB.SetEvent(Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenStoreWindow")


	# AI
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetTooltip (Window, Button, 41631) # or 41646 Activate ...
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenFloatMenuWindow")

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


PokWindow = None
def CntReachPress ():
	print "CntReachPress"
	global PokWindow
	
	GemRB.HideGUI()
	
	if PokWindow != None:
		
		GemRB.UnloadWindow(PokWindow)
		PokWindow = None
		GemRB.SetVar("OtherWindow", -1)
		#SetSelectionChangeHandler (None)
		GemRB.UnhideGUI()
		return
		
	GemRB.LoadWindowPack ("GUIWORLD")
	#PokWindow = Window = GemRB.CreateWindow (100, 0, 0, 640, 480, "GUIID")
	PokWindow = Window = GemRB.CreateWindow (100, 0, 0, 640, 480, "GUILS05")
        GemRB.SetVar("OtherWindow", PokWindow)

	GemRB.UnhideGUI ()


def AIPress ():
	print "AIPress"

def TxtePress ():
	print "TxtePress"

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
	


# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	#return GemRB.GetPlayerPortrait( actor, which)

	PortraitTable = GemRB.LoadTable ("PDOLLS")
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	row = "0x%04X" %anim_id

	return GemRB.GetTableValue (PortraitTable, row, which)
	


SelectionChangeHandler = None

def SetSelectionChangeHandler (handler):
	global SelectionChangeHandler
	SelectionChangeHandler = handler

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()

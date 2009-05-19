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


# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB

from GUICommonWindows import *
import GUICommonWindows
from GUIClasses import GTextArea

from FloatMenuWindow import *

from GUIJRNL import *
from GUIMA import *
from GUIMG import *
from GUIINV import *
from GUIOPT import *
from GUIPR import *
from GUIREC import *
from GUISTORE import *
from GUIWORLD import *
from GUISAVE import *

MessageWindow = 0
ActionsWindow = 0
PortraitWindow = 0
OptionsWindow = 0

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)
	GemRB.LoadWindowPack (GetWindowPack())
	GemRB.SetInfoTextColor(0,255,0,255)
	ActionsWindow = GemRB.LoadWindowObject(0)
	OptionsWindow = GemRB.LoadWindowObject(2)
	MessageWindow = GemRB.LoadWindowObject(7)
	PortraitWindow = OpenPortraitWindow (1)

	MessageTA = MessageWindow.GetControl (1)
	MessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	MessageTA.SetHistory (100)
	GemRB.SetVar ("MessageTextArea", MessageTA.ID)
	GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
	GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVar ("ActionsPosition", 1) #Bottom
	GemRB.SetVar ("OptionsPosition", 1) #Bottom
	GemRB.SetVar ("MessagePosition", 1) #Bottom
	GemRB.SetVar ("OtherPosition", 0) #Left
	
	GemRB.GameSetScreenFlags (0, OP_SET)
	
	CloseButton= MessageWindow.GetControl (0)
	CloseButton.SetText(28082)
	CloseButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	
	OpenButton = OptionsWindow.GetControl (10)
	OpenButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")

	# Select all
	Button = ActionsWindow.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SelectAllOnPress")

	# Select all
	Button = ActionsWindow.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "StopAllOnPress")

	FormationButton = ActionsWindow.GetControl (4)
	FormationButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenFormationWindow")

	SetupActionsWindowControls (ActionsWindow)
	SetupMenuWindowControls (OptionsWindow)

	UpdateControlStatus ()

	
def OnIncreaseSize():
	GemRB.GameSetScreenFlags (GS_LARGEDIALOG, OP_OR)


def OnDecreaseSize():
	GemRB.GameSetScreenFlags (GS_LARGEDIALOG, OP_NAND)


def UpdateControlStatus ():
	global MessageWindow, PortraitWindow, ActionsWindow, OptionsWindow

	Expand = GemRB.GetMessageWindowSize() & (GS_DIALOGMASK|GS_DIALOG)

	hideflags = GemRB.HideGUI ()
	if Expand:
		GemRB.SetVar ("MessageWindow", MessageWindow.ID)
		GemRB.SetVar ("PortraitWindow", -1)
		GemRB.SetVar ("ActionsWindow", -1)
		GemRB.SetVar ("OptionsWindow", -1)
		MessageTA = GTextArea(MessageWindow.ID, GemRB.GetVar ("MessageTextArea"))
		MessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GemRB.SetVar ("MessageWindow", -1)
		GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
		GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
		GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
		GemRB.SetControlStatus (0,0,IE_GUI_CONTROL_FOCUSED)

	if hideflags:
		GemRB.UnhideGUI ()


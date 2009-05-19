# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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
from GUICommon import GameIsHOW

from GUIJRNL import *
from GUIMA import *
from GUIMG import *
from GUIINV import *
from GUIOPT import *
from GUIPR import *
from GUIREC import *
from GUISTORE import *
from GUIWORLD import *
from TextScreen import *

MessageWindow = 0
PortraitWindow = 0
OptionsWindow = 0
ExpandButton = 0
ContractButton = 0

def OnLoad():
	global PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize (PARTY_SIZE)
	GemRB.GameSetProtagonistMode (2)
	GemRB.LoadWindowPack (GetWindowPack())

	GUICommonWindows.PortraitWindow = None
	GUICommonWindows.ActionsWindow = None
	GUICommonWindows.OptionsWindow = None

	#this is different in IWD (0) and HoW (25)
	if GameIsHOW():
		OptionsWindow = GemRB.LoadWindowObject (25)
	else:
		OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 1, "ReturnToGame")
	PortraitWindow = OpenPortraitWindow (1)

	ActionsWindow = GemRB.LoadWindowObject (3)
	OpenActionsWindowControls (ActionsWindow)

	GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
	GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
	GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
	GemRB.SetVar ("TopWindow", -1)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVar ("FloatWindow", -1)
	GemRB.SetVar ("PortraitPosition", 2) #Right
	GemRB.SetVar ("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar ("OptionsPosition", 0) #Left
	GemRB.SetVar ("MessagePosition", 4) #BottomAdded
	GemRB.SetVar ("OtherPosition", 5) #Inactivating
	GemRB.SetVar ("TopPosition", 5) #Inactivating
	
	UpdateControlStatus ()

def OnIncreaseSize ():
	GSFlags = GemRB.GetMessageWindowSize ()
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand>2:
		return
	Expand = (Expand + 1)*2
	GemRB.GameSetScreenFlags (Expand + GSFlags, OP_SET)

def OnDecreaseSize ():
	GSFlags = GemRB.GetMessageWindowSize ()
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand<2:
		return
	Expand = Expand/2 - 1 # 6->2, 2->0
	GemRB.GameSetScreenFlags (Expand + GSFlags, OP_SET)

def UpdateControlStatus ():
	global MessageWindow, ExpandButton, ContractButton
	
	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize ()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand
	
	if Override:
		Expand = GS_LARGEDIALOG

	MessageWindow = GemRB.GetVar ("MessageWindow")

	GemRB.LoadWindowPack (GetWindowPack())
	
	if Expand == GS_MEDIUMDIALOG:
		TMessageWindow = GemRB.LoadWindowObject (12)
		TMessageTA = TMessageWindow.GetControl (1)
		ExpandButton = TMessageWindow.GetControl (0)
		ExpandButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")
		ContractButton = TMessageWindow.GetControl (3)
		ContractButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")

	elif Expand == GS_LARGEDIALOG:
		TMessageWindow = GemRB.LoadWindowObject (7)
		TMessageTA = TMessageWindow.GetControl (1)
		ContractButton = TMessageWindow.GetControl (0)
		ContractButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	else:
		TMessageWindow = GemRB.LoadWindowObject (4)
		TMessageTA = TMessageWindow.GetControl (3)
		ExpandButton = TMessageWindow.GetControl (2)
		ExpandButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")

	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL)
	TMessageTA.SetHistory (100)

	hideflag = GemRB.HideGUI ()
	MessageTA = GTextArea (MessageWindow,GemRB.GetVar ("MessageTextArea"))
	if MessageWindow>0 and MessageWindow!=TMessageWindow.ID:
		MessageTA.MoveText (TMessageTA)
		GemRB.UnloadWindow (MessageWindow)

	GemRB.SetVar ("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar ("MessageTextArea", TMessageTA.ID)
	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GemRB.SetControlStatus (0,0,IE_GUI_CONTROL_FOCUSED)

	if hideflag:
		GemRB.UnhideGUI ()


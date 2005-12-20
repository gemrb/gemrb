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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/how/MessageWindow.py,v 1.25 2005/12/20 20:14:02 avenger_teambg Exp $

# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB
from GUICommonWindows import *
import GUICommonWindows

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

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(2)
	GemRB.LoadWindowPack(GetWindowPack())

	GUICommonWindows.PortraitWindow = None
	GUICommonWindows.ActionsWindow = None
	GUICommonWindows.OptionsWindow = None

	#this is different in IWD and HoW
	OptionsWindow = GemRB.LoadWindow(25)
	SetupMenuWindowControls (OptionsWindow, 1, "ReturnToGame")
	PortraitWindow = OpenPortraitWindow(1)

	ActionsWindow = GemRB.LoadWindow(3)
	OpenActionsWindowControls (ActionsWindow)

	GemRB.SetVar("PortraitWindow", PortraitWindow)
	GemRB.SetVar("ActionsWindow", ActionsWindow)
	GemRB.SetVar("OptionsWindow", OptionsWindow)
	GemRB.SetVar("TopWindow", -1)
	GemRB.SetVar("OtherWindow", -1)
	GemRB.SetVar("FloatWindow", -1)
	GemRB.SetVar("PortraitPosition", 2) #Right
	GemRB.SetVar("ActionsPosition", 4) #BottomAdded
	GemRB.SetVar("OptionsPosition", 0) #Left
	GemRB.SetVar("MessagePosition", 4) #BottomAdded
	GemRB.SetVar("OtherPosition", 5) #Inactivating
	GemRB.SetVar("TopPosition", 5) #Inactivating
	
	UpdateControlStatus()

def OnIncreaseSize():
	GSFlags = GemRB.GetVar("MessageWindowSize")
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand>2:
		return
	Expand = (Expand + 1)*2
	GemRB.GameSetScreenFlags(Expand + GSFlags, OP_SET)

def OnDecreaseSize():
	GSFlags = GemRB.GetVar("MessageWindowSize")
	Expand = GSFlags&GS_DIALOGMASK
	GSFlags = GSFlags-Expand
	if Expand<2:
		return
	Expand = Expand/2 - 1 # 6->2, 2->0
	GemRB.GameSetScreenFlags(Expand + GSFlags, OP_SET)

def UpdateControlStatus():
	global MessageWindow, ExpandButton, ContractButton
	
	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetVar("MessageWindowSize")
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand
	
	if Override:
		Expand = GS_LARGEDIALOG

	MessageWindow = GemRB.GetVar("MessageWindow")

	GemRB.LoadWindowPack(GetWindowPack())
	
	if Expand == GS_MEDIUMDIALOG:
		TMessageWindow = GemRB.LoadWindow(12)
		TMessageTA = GemRB.GetControl(TMessageWindow, 1)
		ExpandButton = GemRB.GetControl(TMessageWindow, 0)
		GemRB.SetEvent(TMessageWindow, ExpandButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")
		ContractButton = GemRB.GetControl(TMessageWindow, 3)
		GemRB.SetEvent(TMessageWindow, ContractButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")

	elif Expand == GS_LARGEDIALOG:
		TMessageWindow = GemRB.LoadWindow(7)
		TMessageTA = GemRB.GetControl(TMessageWindow, 1)
		ContractButton = GemRB.GetControl(TMessageWindow, 0)
		GemRB.SetEvent(TMessageWindow, ContractButton, IE_GUI_BUTTON_ON_PRESS, "OnDecreaseSize")
	else:
		TMessageWindow = GemRB.LoadWindow(4)
		TMessageTA = GemRB.GetControl(TMessageWindow, 3)
		ExpandButton = GemRB.GetControl(TMessageWindow, 2)
		GemRB.SetEvent(TMessageWindow, ExpandButton, IE_GUI_BUTTON_ON_PRESS, "OnIncreaseSize")

	GemRB.SetTextAreaFlags(TMessageWindow, TMessageTA, IE_GUI_TEXTAREA_AUTOSCROLL)
	GemRB.SetTAHistory(TMessageWindow, TMessageTA, 100)

	hideflag = GemRB.HideGUI()
	MessageTA = GemRB.GetVar("MessageTextArea")
	if MessageWindow>0 and MessageWindow!=TMessageWindow:
		GemRB.MoveTAText(MessageWindow, MessageTA, TMessageWindow, TMessageTA)
		GemRB.UnloadWindow(MessageWindow)

	GemRB.SetVar("MessageWindow", TMessageWindow)
	GemRB.SetVar("MessageTextArea", TMessageTA)

	if hideflag:
		GemRB.UnhideGUI()


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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

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
def OnLoad():
	global MessageWindow, PortraitWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(2)
	GemRB.SetDefaultActions(1,14,16,17)
	GemRB.LoadWindowPack(GetWindowPack())
	OptionsWindow = MessageWindow = GemRB.LoadWindow(0)
	ActionsWindow = PortraitWindow = OpenPortraitWindow(1)

	GemRB.SetVar ("MessageWindow", MessageWindow)
	GemRB.SetVar ("PortraitWindow", PortraitWindow)
	GemRB.SetVar ("TopWindow", -1)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVar ("FloatWindow", -1)
	GemRB.SetVar ("ActionsPosition", -1) #already handled in portraitwindow
	GemRB.SetVar ("OptionsPosition", -1) #already handled in messagewindow
	GemRB.SetVar ("MessagePosition", 4)
	GemRB.SetVar ("PortraitPosition", 4)
	GemRB.SetVar ("OtherPosition", 5) #Inactivating
	GemRB.SetVar ("TopPosition", 5) #Inactivating

	SetupMenuWindowControls (OptionsWindow, 1, "ReturnToGame")

	GemRB.SetVisible(MessageWindow, 1)
	GemRB.SetVisible(PortraitWindow, 1)
	return

def OnIncreaseSize():
	GemRB.GameSetScreenFlags(GS_LARGEDIALOG, OP_OR)

def OnDecreaseSize():
	GemRB.GameSetScreenFlags(GS_LARGEDIALOG, OP_NAND)

def UpdateControlStatus():
	global MessageWindow

	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetVar ("MessageWindowSize")
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	#a dialogue is running, setting messagewindow size to maximum
	if Override:
		Expand = GS_LARGEDIALOG

	MessageWindow = GemRB.GetVar ("MessageWindow")

	GemRB.LoadWindowPack(GetWindowPack())
	hideflag = GemRB.HideGUI()
	if Expand == GS_LARGEDIALOG:
		GemRB.SetVar ("PortraitWindow", -1)
		TMessageWindow = GemRB.LoadWindow(7)
		TMessageTA = GemRB.GetControl (TMessageWindow, 1)
	else:
		GemRB.SetVar ("PortraitWindow", PortraitWindow)
		TMessageWindow = GemRB.LoadWindow(0)
		TMessageTA = GemRB.GetControl (TMessageWindow, 1)


	GemRB.SetTextAreaFlags(TMessageWindow, TMessageTA, IE_GUI_TEXTAREA_AUTOSCROLL)
	GemRB.SetTAHistory(TMessageWindow, TMessageTA, 100)

	MessageTA = GemRB.GetVar ("MessageTextArea")
	if MessageWindow>0 and MessageWindow!=TMessageWindow:
		GemRB.MoveTAText (MessageWindow, MessageTA, TMessageWindow, TMessageTA)
		GemRB.UnloadWindow(MessageWindow)
	GemRB.SetVar ("MessageWindow", TMessageWindow)
	GemRB.SetVar ("MessageTextArea", TMessageTA)

	if hideflag:
		GemRB.UnhideGUI()
	return


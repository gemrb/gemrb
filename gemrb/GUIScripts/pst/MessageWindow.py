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


# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB
import GUIClasses
import GUICommon
import GUICommonWindows
import CommonWindow
import GUIWORLD
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

MessageWindow = 0
ActionsWindow = 0
PortraitWindow = 0
OptionsWindow = 0
MessageTA = 0

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(MAX_PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)
	GemRB.SetInfoTextColor({'r' : 0, 'g' : 255, 'b' : 0, 'a' : 255})

	ActionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack())
	ActionsWindow.AddAlias("ACTWIN")
	ActionsWindow.AddAlias("HIDE_CUT", 1)
	ActionsWindow.AddAlias("NOT_DLG", 0)

	OptionsWindow = GemRB.LoadWindow(2)
	OptionsWindow.AddAlias("OPTWIN")
	OptionsWindow.AddAlias("HIDE_CUT", 2)
	OptionsWindow.AddAlias("NOT_DLG", 1)

	MessageWindow = GemRB.LoadWindow(7)
	MessageWindow.SetFlags(WF_DESTROY_ON_CLOSE, OP_NAND)
	MessageWindow.AddAlias("MSGWIN")
	MessageWindow.AddAlias("HIDE_CUT", 0)

	PortraitWindow = GUICommonWindows.OpenPortraitWindow (1)
	PortraitWindow.AddAlias("PORTWIN")
	PortraitWindow.AddAlias("HIDE_CUT", 3)
	PortraitWindow.AddAlias("NOT_DLG", 2)

	MessageTA = MessageWindow.GetControl (1)
	MessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	MessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	MessageTA.AddAlias("MsgSys", 0)
	
	GemRB.GameSetScreenFlags (0, OP_SET)
	
	CloseButton= MessageWindow.GetControl (0)
	CloseButton.SetText(28082)
	CloseButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: MessageWindow.Close())
	CloseButton.MakeDefault()
	
	OpenButton = OptionsWindow.GetControl (10)
	OpenButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: MessageWindow.Focus())

	# Select all
	Button = ActionsWindow.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectAllOnPress)

	# Select all
	Button = ActionsWindow.GetControl (3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.ActionStopPressed)

	FormationButton = ActionsWindow.GetControl (4)
	FormationButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIWORLD.OpenFormationWindow)

	GUICommonWindows.SetupClockWindowControls (ActionsWindow)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow)

	UpdateControlStatus ()

def UpdateControlStatus ():
	if GemRB.GetGUIFlags() & (GS_DIALOGMASK|GS_DIALOG):
		Label = MessageWindow.GetControl (0x10000003)
		Label.SetText (str (GemRB.GameGetPartyGold ()))


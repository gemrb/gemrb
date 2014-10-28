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
from GUIDefines import *

MessageWindow = 0
ActionsWindow = 0
PortraitWindow = 0
OptionsWindow = 0
MessageTA = 0

def OnLoad():
	global MessageWindow, ActionsWindow, PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(0)
	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	GemRB.SetInfoTextColor(0,255,0,255)
	ActionsWindow = GemRB.LoadWindow(0)
	OptionsWindow = GemRB.LoadWindow(2)
	MessageWindow = GemRB.LoadWindow(7)
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (1)

	MessageTA = MessageWindow.GetControl (1)
	MessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)

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
	CloseButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnDecreaseSize)
	CloseButton.SetFlags (IE_GUI_BUTTON_DEFAULT | IE_GUI_BUTTON_MULTILINE, OP_OR)
	
	OpenButton = OptionsWindow.GetControl (10)
	OpenButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnIncreaseSize)

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
	global MessageWindow, PortraitWindow, ActionsWindow, OptionsWindow, MessageTA

	Expand = GemRB.GetMessageWindowSize() & (GS_DIALOGMASK|GS_DIALOG)

	hideflags = GemRB.HideGUI ()
	if Expand:
		GemRB.SetVar ("MessageWindow", MessageWindow.ID)
		GemRB.SetVar ("PortraitWindow", -1)
		GemRB.SetVar ("ActionsWindow", -1)
		GemRB.SetVar ("OptionsWindow", -1)
		MessageTA = GUIClasses.GTextArea(MessageWindow.ID, GemRB.GetVar ("MessageTextArea"))
		MessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
		
		Label = MessageWindow.GetControl (0x10000003)
		Label.SetText (str (GemRB.GameGetPartyGold ()))
	else:
		GemRB.SetVar ("MessageWindow", -1)
		GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
		GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
		GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)

	if hideflags:
		GemRB.UnhideGUI ()


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

# MessageWindow.py - scripts and GUI for main (walk) window

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
import CommonWindow
from GUIDefines import *
import GUIClasses

MessageWindow = 0
PortraitWindow = 0
OptionsWindow = 0
ExpandButton = 0
ContractButton = 0
TMessageTA = 0 # for dialog code

def OnLoad():
	global PortraitWindow, OptionsWindow

	GemRB.GameSetPartySize (PARTY_SIZE)
	GemRB.GameSetProtagonistMode (2)
	GemRB.LoadWindowPack (GUICommon.GetWindowPack())

	GUICommonWindows.PortraitWindow = None
	GUICommonWindows.ActionsWindow = None
	GUICommonWindows.OptionsWindow = None

	#this is different in IWD (0) and HoW (25)
	if GameCheck.HasHOW():
		OptionsWindow = GemRB.LoadWindow (25)
	else:
		OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (1)

	ActionsWindow = GemRB.LoadWindow (3)
	GUICommonWindows.OpenActionsWindowControls (ActionsWindow)

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

def UpdateControlStatus ():
	global MessageWindow, ExpandButton, ContractButton, TMessageTA
	
	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize ()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand
	
	if Override:
		Expand = GS_LARGEDIALOG

	MessageWindow = GemRB.GetVar ("MessageWindow")

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	
	if Expand == GS_MEDIUMDIALOG:
		TMessageWindow = GemRB.LoadWindow (12)
		TMessageTA = TMessageWindow.GetControl (1)
		ExpandButton = TMessageWindow.GetControl (0)
		ExpandButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnIncreaseSize)
		ContractButton = TMessageWindow.GetControl (3)
		ContractButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnDecreaseSize)

	elif Expand == GS_LARGEDIALOG:
		TMessageWindow = GemRB.LoadWindow (7)
		TMessageTA = TMessageWindow.GetControl (1)
		ContractButton = TMessageWindow.GetControl (0)
		ContractButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnDecreaseSize)
	else:
		TMessageWindow = GemRB.LoadWindow (4)
		TMessageTA = TMessageWindow.GetControl (3)
		ExpandButton = TMessageWindow.GetControl (2)
		ExpandButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CommonWindow.OnIncreaseSize)

	TMessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)

	hideflag = GemRB.HideGUI ()
	MessageTA = GUIClasses.GTextArea (MessageWindow,GemRB.GetVar ("MessageTextArea"))
	if MessageWindow>0 and MessageWindow!=TMessageWindow.ID:
		MessageTA.MoveText (TMessageTA)
		GUIClasses.GWindow(MessageWindow).Unload()

	GemRB.SetVar ("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar ("MessageTextArea", TMessageTA.ID)
	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
	else:
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)

	if hideflag:
		GemRB.UnhideGUI ()
	return

#upgrade savegame to next version
def GameExpansion():
	#the original savegames got 0, but the engine upgrades all saves to 3
	#this is a good place to perform one-time adjustments if needed
	GemRB.GameSetExpansion(3)
	return


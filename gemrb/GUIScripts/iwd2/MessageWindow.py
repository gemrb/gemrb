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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

import GemRB
import GUICommon
import GUICommonWindows
import GUIClasses
from GUIDefines import *

MessageWindow = 0
PortraitWindow = 0
TMessageTA = 0 # for dialog code

def OnLoad():
	global MessageWindow, PortraitWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(2)
	GemRB.SetDefaultActions(1,14,16,17)
	GemRB.LoadWindowPack(GUICommon.GetWindowPack())
	OptionsWindow = MessageWindow = GemRB.LoadWindow(0)
	ActionsWindow = PortraitWindow = GUICommonWindows.OpenPortraitWindow()

	GemRB.SetVar ("MessageWindow", MessageWindow.ID)
	GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
	GemRB.SetVar ("ActionsWindow", 12345) # dummy so the EF_ACTION callback works
	GemRB.SetVar ("TopWindow", -1)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVar ("FloatWindow", -1)
	GemRB.SetVar ("ActionsPosition", -1) #already handled in portraitwindow
	GemRB.SetVar ("OptionsPosition", -1) #already handled in messagewindow
	GemRB.SetVar ("MessagePosition", 4)
	GemRB.SetVar ("PortraitPosition", 4)
	GemRB.SetVar ("OtherPosition", 5) #Inactivating
	GemRB.SetVar ("TopPosition", 5) #Inactivating

	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)

	MessageWindow.SetVisible(WINDOW_VISIBLE)
	PortraitWindow.SetVisible(WINDOW_VISIBLE)
	return

def UpdateControlStatus():
	global MessageWindow,TMessageTA

	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	#a dialogue is running, setting messagewindow size to maximum
	if Override:
		Expand = GS_LARGEDIALOG

	GemRB.LoadWindowPack(GUICommon.GetWindowPack())
	hideflag = GemRB.HideGUI()
	if Expand == GS_LARGEDIALOG:
		GemRB.SetVar ("PortraitWindow", -1)
		TMessageWindow = GemRB.LoadWindow(7)
		TMessageTA = TMessageWindow.GetControl (1)
	else:
		GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
		TMessageWindow = GemRB.LoadWindow(0)
		# cheat code editbox; only causes redraw issues if you click on the lower left
		TMessageWindow.DeleteControl (3)
		TMessageTA = TMessageWindow.GetControl (1)
		GUICommonWindows.SetupMenuWindowControls (TMessageWindow, 1, None)

	MessageWindow = GemRB.GetVar ("MessageWindow")
	MessageTA = GUIClasses.GTextArea(MessageWindow, GemRB.GetVar ("MessageTextArea"))
	if MessageWindow > 0 and MessageWindow != TMessageWindow.ID:
		TMessageTA = MessageTA.SubstituteForControl(TMessageTA)
		GUIClasses.GWindow(MessageWindow).Unload()
		
	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	GemRB.SetVar ("MessageWindow", TMessageWindow.ID)
	GemRB.SetVar ("MessageTextArea", TMessageTA.ID)

	if Override:
		TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)
		#gets PC currently talking
		pc = GemRB.GameGetSelectedPCSingle (1)
		if pc:
			Portrait = GemRB.GetPlayerPortrait(pc,1)
		else:
			Portrait = None
		Button = TMessageWindow.GetControl(11)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		if not Portrait:
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.SetPicture(Portrait, "NOPORTSM")
	else:
		GUICommon.GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)
		
	if hideflag:
		GemRB.UnhideGUI()
	return

#upgrade savegame to next version
def GameExpansion():
	#the original savegames got 0, but the engine upgrades all saves to 3
	#this is a good place to perform one-time adjustments if needed
	GemRB.GameSetExpansion(3)
	return


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

from GUICommonWindows import *
import GUICommonWindows
from GUICommon import GameControl
from GUIINV import *
from GUIJRNL import *
from GUIMA import *
from GUIOPT import *
from GUISPL import *
from GUIREC import *
from GUISTORE import *
from GUIWORLD import *
from TextScreen import *
from GUIClasses import GTextArea
from GUIClasses import GWindow
from CommonWindow import OnIncreaseSize, OnDecreaseSize

MessageWindow = 0
PortraitWindow = 0
def OnLoad():
	global MessageWindow, PortraitWindow

	GemRB.GameSetPartySize(PARTY_SIZE)
	GemRB.GameSetProtagonistMode(2)
	GemRB.SetDefaultActions(1,14,16,17)
	GemRB.LoadWindowPack(GetWindowPack())
	OptionsWindow = MessageWindow = GemRB.LoadWindowObject(0)
	ActionsWindow = PortraitWindow = OpenPortraitWindow()

	GemRB.SetVar ("MessageWindow", MessageWindow.ID)
	GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
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

	MessageWindow.SetVisible(WINDOW_VISIBLE)
	PortraitWindow.SetVisible(WINDOW_VISIBLE)
	return

def UpdateControlStatus():
	global MessageWindow

	TMessageWindow = 0
	TMessageTA = 0
	GSFlags = GemRB.GetMessageWindowSize()
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
		TMessageWindow = GemRB.LoadWindowObject(7)
		TMessageTA = TMessageWindow.GetControl (1)
	else:
		GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
		TMessageWindow = GemRB.LoadWindowObject(0)
		TMessageTA = TMessageWindow.GetControl (1)
		SetupMenuWindowControls (TMessageWindow, 1, "ReturnToGame")


	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL)
	TMessageTA.SetHistory(100)

	MessageTA = GTextArea(MessageWindow, GemRB.GetVar ("MessageTextArea"))
	if MessageWindow>0 and MessageWindow!=TMessageWindow.ID:
		MessageTA.MoveText (TMessageTA)
		GWindow(MessageWindow).Unload()
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
		GameControl.SetStatus(IE_GUI_CONTROL_FOCUSED)
		
	if hideflag:
		GemRB.UnhideGUI()
	return


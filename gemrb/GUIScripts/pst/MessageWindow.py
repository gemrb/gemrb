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
import Clock
import PortraitWindow
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

MWindow = 0

def OnLoad():
	global MWindow

	# TODO: we can uncomment the "HIDE_CUT" lines below to hide the windows for cutscenes
	# the original doesn't hide them and it looks like there is a map drawing bug at the bottom of the screen due to the bottom
	# row of tiles getting squished for not fitting perfectly on screen (tho I havent seen this in BG2, but maybe wasnt paying attention)

	ActionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_LEFT)
	ActionsWindow.AddAlias("ACTWIN")
	#ActionsWindow.AddAlias("HIDE_CUT", 1)
	ActionsWindow.AddAlias("NOT_DLG", 0)
	ActionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	OptionsWindow = GemRB.LoadWindow(2, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_RIGHT)
	OptionsWindow.AddAlias("OPTWIN")
	#OptionsWindow.AddAlias("HIDE_CUT", 2)
	OptionsWindow.AddAlias("NOT_DLG", 1)
	OptionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	GemRB.SetVar ("PortraitPosition", 1)

	MWindow = GemRB.LoadWindow(7, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MWindow.SetFlags(WF_DESTROY_ON_CLOSE, OP_NAND)
	MWindow.AddAlias("MSGWIN")
	MWindow.AddAlias("HIDE_CUT", 0)
	MWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	PortraitWin = PortraitWindow.OpenPortraitWindow (WINDOW_BOTTOM|WINDOW_HCENTER)
	PortraitWin.AddAlias("NOT_DLG", 2)
	PortraitWin.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	pframe = PortraitWin.GetFrame()
	pframe['x'] -= 16
	PortraitWin.SetFrame(pframe)

	MessageTA = MWindow.GetControl (1)
	MessageTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	MessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	MessageTA.AddAlias("MsgSys", 0)
	MessageTA.SetColor(ColorRed, TA_COLOR_OPTIONS)
	MessageTA.SetColor(ColorWhite, TA_COLOR_HOVER)

	CloseButton = MWindow.GetControl (0)
	CloseButton.SetText(28082)
	CloseButton.OnPress (MWindow.Close)
	CloseButton.MakeDefault()
	
	OpenButton = OptionsWindow.GetControl (10)
	OpenButton.OnPress (MWindow.Focus)

	SetupClockWindowControls (ActionsWindow)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow)

	UpdateControlStatus ()
	
def SetupClockWindowControls (Window):
	# time button
	Button = Window.GetControl (0)
	Clock.CreateClockButton(Button)

	# 41627 - Return to the Game World
	Button = Window.GetControl (2)
	Button.OnPress (GUICommonWindows.CloseTopWindow)
	Button.SetTooltip (41627)

	# Select all characters
	Button = Window.GetControl (1)
	Button.SetTooltip (41659)
	Button.OnPress (GUICommon.SelectAllOnPress)

	# Abort current action
	Button = Window.GetControl (3)
	Button.SetTooltip (41655)
	Button.OnPress (GUICommonWindows.ActionStopPressed)

	# Formations
	import GUIWORLD
	Button = Window.GetControl (4)
	Button.SetTooltip (44945)
	Button.OnPress (GUIWORLD.OpenFormationWindow)

	return

def UpdateControlStatus ():
	if GemRB.GetGUIFlags() & (GS_DIALOGMASK|GS_DIALOG):
		Label = MWindow.GetControl (0x10000003)
		Label.SetText (str (GemRB.GameGetPartyGold ()))

		MWindow.Focus()
	elif MWindow:
		MWindow.Close()


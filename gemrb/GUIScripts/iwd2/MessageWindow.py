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
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *
import CommonWindow

def OnLoad():
	OptionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack())
	OptionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	OptionsWindow.AddAlias("OPTWIN")
	OptionsWindow.AddAlias("HIDE_CUT", 2)
	OptionsWindow.AddAlias("NOT_DLG", 1)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)

	ActionsWindow = GUICommonWindows.OpenPortraitWindow(0, WINDOW_BOTTOM|WINDOW_HCENTER)
	ActionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	ActionsWindow.AddAlias("ACTWIN")
	ActionsWindow.AddAlias("HIDE_CUT", 1)
	ActionsWindow.AddAlias("NOT_DLG", 0)

	actframe = ActionsWindow.GetFrame()
	optframe = OptionsWindow.GetFrame()

	actframe['y'] -= optframe['h']
	ActionsWindow.SetFrame(actframe)

	return

def UpdateControlStatus():
	OptionsWindow = GemRB.GetView("OPTWIN")

	if not OptionsWindow:
		# exit if we get called from core without the right window pack being loaded
		return

	GSFlags = GemRB.GetGUIFlags()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	#a dialogue is running, setting messagewindow size to maximum
	if Override:
		Expand = GS_LARGEDIALOG

	if Expand == GS_LARGEDIALOG:
		MessageWindow = GemRB.LoadWindow(7, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	else:
		MessageWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
		MessageWindow.AddAlias("OPTWIN")
		GUICommonWindows.SetupMenuWindowControls (MessageWindow, 1, None)

	MessageWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	MessageWindow.AddAlias("MSGWIN")
	MessageWindow.AddAlias("HIDE_CUT", 0)

	# FIXME: I dont know what this TextEdit is for
	# if we need it, then we should hide it until it is used because it is being drawn in the same area as the MEssageTA
	te = MessageWindow.GetControl(3)
	if te != None:
		MessageWindow.RemoveSubview(te)

	MessageTA = MessageWindow.GetControl(1)
	MessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	MessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	MessageTA.AddAlias("MsgSys", 0)

	# compensate for the bogus scrollbar on the CHU BG
	frame = MessageTA.GetFrame()
	frame['w'] += 12
	frame['h'] += 4;
	frame['y'] -= 4;
	MessageTA.SetFrame(frame)

	if Override:
		#gets PC currently talking
		pc = GemRB.GameGetSelectedPCSingle (1)
		if pc:
			Portrait = GemRB.GetPlayerPortrait(pc, 1)["Sprite"]
		else:
			Portrait = None
		Button = MessageWindow.GetControl(11)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		if not Portrait:
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.SetPicture(Portrait, "NOPORTSM")
		
	return




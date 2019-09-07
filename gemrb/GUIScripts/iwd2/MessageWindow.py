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

MTASize = GS_SMALLDIALOG

def OnLoad():
	OptionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	OptionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	OptionsWindow.AddAlias("OPTWIN")
	OptionsWindow.AddAlias("HIDE_CUT", 2)
	OptionsWindow.AddAlias("NOT_DLG", 1)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)

	# this also has the small MTA window
	OptionsWindow.AddAlias("MSGWIN")
	# compensate for the bogus scrollbar on the CHU BG
	smallMTA = OptionsWindow.GetControl(1)
	smallMTA.AddAlias("MsgSys", 0)
	smallMTA.AddAlias("MTA_SM", 0)

	frame = smallMTA.GetFrame()
	frame['w'] += 12
	frame['h'] += 4
	frame['y'] -= 4
	smallMTA.SetFrame(frame)
	smallMTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)

	# FIXME: I dont know what this TextEdit is for
	# if we need it, then we should hide it until it is used because it is being drawn in the same area as the MEssageTA
	OptionsWindow.RemoveSubview(OptionsWindow.GetControl(3))

	ActionsWindow = GUICommonWindows.OpenPortraitWindow(0, WINDOW_BOTTOM|WINDOW_HCENTER)
	ActionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	ActionsWindow.AddAlias("ACTWIN")
	ActionsWindow.AddAlias("HIDE_CUT", 1)
	ActionsWindow.AddAlias("NOT_DLG", 0)

	actframe = ActionsWindow.GetFrame()
	optframe = OptionsWindow.GetFrame()

	actframe['y'] -= optframe['h']
	ActionsWindow.SetFrame(actframe)

	DialogWindow = GemRB.LoadWindow(7, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	DialogWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS|IE_GUI_VIEW_INVISIBLE, OP_OR)
	DialogWindow.AddAlias("MSGWINLG")
	DialogWindow.AddAlias("HIDE_CUT", 0)
	DialogWindow.AddAlias("NOT_DLG", 2)

	largeMTA = DialogWindow.GetControl(1)
	largeMTA.AddAlias("MTA_LG", 0)

	frame = largeMTA.GetFrame()
	frame['w'] += 34
	frame['h'] += 3
	frame['y'] -= 2
	largeMTA.SetFrame(frame)

	return

def UpdateControlStatus():
	global MTASize

	GSFlags = GemRB.GetGUIFlags()
	Expand = GSFlags&GS_DIALOGMASK
	Override = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	largeMW = GemRB.GetView("MSGWINLG")

	#a dialogue is running, setting messagewindow size to maximum
	if Override:
		Expand = GS_LARGEDIALOG
		largeMW.SetVisible(True)
		largeMW.SetDisabled(False)
	else:
		largeMW.SetVisible(False)

	if MTASize == Expand:
		return

	MTASize = Expand

	smallMW = GemRB.GetView("MSGWIN")
	smallMTA = GemRB.GetView("MTA_SM", 0)
	smallFrame = smallMTA.GetFrame()

	largeMTA = GemRB.GetView("MTA_LG", 0)
	largeFrame = largeMTA.GetFrame()

	# swap the 2 TextAreas
	largeMW.AddSubview(smallMTA)
	smallMTA.SetFrame(largeFrame)
	smallMTA.AddAlias("MTA_LG", 0)
	
	smallMW.AddSubview(largeMTA)
	largeMTA.SetFrame(smallFrame)
	largeMTA.AddAlias("MTA_SM", 0)
	
	if Override:
		#gets PC currently talking
		pc = GemRB.GameGetSelectedPCSingle (1)
		if pc:
			Portrait = GemRB.GetPlayerPortrait(pc, 1)["Sprite"]
		else:
			Portrait = None
		Button = largeMW.GetControl(11)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		if not Portrait:
			Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		else:
			Button.SetPicture(Portrait, "NOPORTSM")
		
	return


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

MTASize = GS_SMALLDIALOG
smallID = -1
largeID = -1

def OnLoad():
	global smallID, largeID
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
	smallMTA.AddAlias("MTA_SM", 0, True)
	smallMTA.SetFlags (IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	smallMTA.SetColor(ColorRed, TA_COLOR_OPTIONS)
	smallMTA.SetColor(ColorWhite, TA_COLOR_HOVER)
	smallID = smallMTA.ID
	
	sbar = OptionsWindow.GetControl(2)
	sbar.SetResizeFlags(IE_GUI_VIEW_RESIZE_VERTICAL | IE_GUI_VIEW_RESIZE_RIGHT)

	# remove the cheat input textedit
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
	largeMTA.AddAlias("MTA_LG", 0, True)
	largeID = largeMTA.ID

	return

def UpdateControlStatus():
	global MTASize

	GSFlags = GemRB.GetGUIFlags()
	Expand = GSFlags&GS_DIALOGMASK
	InDialog = GSFlags&GS_DIALOG
	GSFlags = GSFlags-Expand

	largeMW = GemRB.GetView("MSGWINLG")

	#a dialogue is running, setting messagewindow size to maximum
	if InDialog:
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
	smallMTA = smallMW.RemoveSubview(smallMTA)
	largeMTA = largeMW.RemoveSubview(largeMTA)

	newLargeMTA = largeMW.AddSubview(smallMTA, None, largeID)
	newLargeMTA.SetFrame(largeFrame)
	newLargeMTA.AddAlias("MTA_LG", 0, True)

	newSmallMTA = smallMW.AddSubview(largeMTA, None, smallID)
	newSmallMTA.SetFrame(smallFrame)
	newSmallMTA.AddAlias("MTA_SM", 0, True)
	
	if InDialog:
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
	else:
		newSmallMTA.ScrollTo(0, -9999) # scroll to the bottom
		
	return


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


# GUIWORLD.py - scripts to control some windows from GUIWORLD winpack
#    except of Actions, Portrait, Options and Dialog windows

###################################################

import GemRB
import GUICommon
import GUICommonWindows
import CommonWindow
import Container
from GUIDefines import *
import MessageWindow

FormationWindow = None
ReformPartyWindow = None

def DialogStarted ():
	global ContinueWindow

	GUICommonWindows.CloseTopWindow()
	Container.CloseContainerWindow()

	# opening control size to maximum, enabling dialog window
	CommonWindow.SetGameGUIHidden(False)
	GemRB.GameSetScreenFlags(GS_DIALOG, OP_OR)
	
	# disable the 1-6 hotkeys, so they'll work for choosing answers
	GUICommonWindows.UpdatePortraitWindow ()

	MWin = GemRB.GetView("MSGWIN")
	CloseButton= MWin.GetControl (0)
	CloseButton.SetText ("")
	CloseButton.SetDisabled(True)
	CloseButton.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

def DialogEnded ():
	Button = MessageWindow.MWindow.GetControl (0)
	Button.MakeDefault(True)
	Button.SetDisabled(False)
	Button.SetText (28082)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

def CloseContinueWindow ():
	# don't close the actual window now to avoid flickering: we might still want it open
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	if not MessageWindow.MWindow:
		return

	Button = MessageWindow.MWindow.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)

def OpenEndMessageWindow ():
	Button = MessageWindow.MWindow.GetControl (0)
	Button.SetText (34602)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	Button.MakeDefault(True)
	Button.SetDisabled(False)

def OpenContinueMessageWindow ():
	#continue
	Button = MessageWindow.MWindow.GetControl (0)
	Button.SetText (34603)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	Button.MakeDefault(True)
	Button.SetDisabled(False)

def OpenReformPartyWindow ():
	global ReformPartyWindow

	ReformPartyWindow = Window = GemRB.LoadWindow (24, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	ReformPartyWindow.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (42514)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	# TODO: implement removal

	# Done
	Button = Window.GetControl (8)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: ReformPartyWindow.Close())


last_formation = None

def OpenFormationWindow ():
	global FormationWindow

	FormationWindow = Window = GemRB.LoadWindow (27, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	FormationWindow.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	# Done
	Button = Window.GetControl (13)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: FormationWindow.Close())

	tooltips = (
		44957,  # Follow
		44958,  # T
		44959,  # Gather
		44960,  # 4 and 2
		44961,  # 3 by 2
		44962,  # Protect
		48152,  # 2 by 3
		44964,  # Rank
		44965,  # V
		44966,  # Wedge
		44967,  # S
		44968,  # Line
		44969,  # None
	)

	for i in range (13):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ("SelectedFormation", i)
		Button.SetTooltip (tooltips[i])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectFormation)

	GemRB.SetVar ("SelectedFormation", GemRB.GameGetFormation (0))
	SelectFormation ()

def SelectFormation ():
	global last_formation
	Window = FormationWindow
	
	formation = GemRB.GetVar ("SelectedFormation")
	print("FORMATION:", formation)
	if last_formation != None and last_formation != formation:
		Button = Window.GetControl (last_formation)
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)

	Button = Window.GetControl (formation)
	Button.SetState (IE_GUI_BUTTON_SELECTED)

	last_formation = formation

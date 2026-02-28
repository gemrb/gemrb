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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIW.py - scripts to control some windows from the GUIWORLD winpack
# except of Actions, Portrait, Options and Dialog windows
#################################################################

import GemRB

import ActionsWindow
import CommonWindow
import Container
import GameCheck
import GUICommon
import GUICommonWindows
from GUIDefines import *

ContinueWindow = None

def OpenDialogButton(id):
	window = GemRB.LoadWindow (id, GUICommon.GetWindowPack())
	window.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	MsgWin = GemRB.GetView("MSGWIN")

	frame = MsgWin.GetFrame()
	offset = 0
	if GameCheck.IsGemRBDemo ():
		offset = window.GetFrame()['h']
	window.SetPos(frame['x'], frame['y'] + frame['h'] - offset)

	# Dialog button shouldnt be in front
	win = GemRB.GetView("OPTWIN")
	if win:
		win.Focus()
	win = GemRB.GetView("PORTWIN")
	if win:
		win.Focus()

	return window

def DialogStarted ():
	global ContinueWindow

	# try to force-close anything which is open
	GUICommonWindows.CloseTopWindow()
	Container.CloseContainerWindow()

	# opening control size to maximum, enabling dialog window
	CommonWindow.SetGameGUIHidden(False)
	GemRB.GameSetScreenFlags(GS_DIALOG, OP_OR)

	ContinueWindow = OpenDialogButton(9)

def DialogEnded ():
	global ContinueWindow

	ActionsWindow.UpdateActionsWindow ()
	if not ContinueWindow:
		return

	# reset the global hotkey, so it doesn't interfere with console use afterwards
	ContinueWindow.GetControl(0).SetHotKey (None, 0, True)
	ContinueWindow.Close ()
	ContinueWindow = None

def CloseContinueWindow ():
	# don't close the actual window now to avoid flickering: we might still want it open
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	if not ContinueWindow:
		return

	ContinueWindow.GetControl(0).SetVisible(False)
	ContinueWindow.GetControl(0).SetDisabled(True)
	GemRB.GetView ("MSGWIN").Focus ()

def OpenEndMessageWindow ():
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	EndDLGStrref = 9371
	if GameCheck.IsGemRBDemo ():
		EndDLGStrref = 67
	Button.SetText (EndDLGStrref)
	Button.OnPress (CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_NO_TOOLTIP, OP_OR)
	Button.MakeDefault(True)

def OpenContinueMessageWindow ():
	# continue button
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	ContinueWindow.Focus ()
	ContinueStrref = 9372
	if GameCheck.IsGemRBDemo ():
		ContinueStrref = 66
	Button.SetText (ContinueStrref)
	Button.OnPress (CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_NO_TOOLTIP, OP_OR)
	Button.MakeDefault(True)

def DeathWindow ():
	# break out of any cutscenes, so we get mouse input back
	# can also happen in "cutscene lite" mode, eg. with wk spirit warrior dream #715
	GemRB.EndCutSceneMode ()
	GemRB.ExecuteString ("SetCursorState(0)")
	if GameCheck.IsIWD1():
		#no death movie, but music is changed
		GemRB.LoadMusicPL ("Theme.mus",1)
	CommonWindow.SetGameGUIHidden(True)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindowEnd ():
	#playing death movie before continuing
	if not GameCheck.IsIWD1():
		GemRB.PlayMovie ("deathand",1)
	GemRB.GamePause (1,3)

	Window = GemRB.LoadWindow (17, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (16498)

	#load
	Button = Window.GetControl (1)
	Button.SetText (15590)
	Button.OnPress (LoadPress)

	#quit
	Button = Window.GetControl (2)
	Button.SetText (15417)
	Button.OnPress (QuitPress)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

def LoadPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return


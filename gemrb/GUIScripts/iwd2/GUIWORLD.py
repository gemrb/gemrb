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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIW.py - scripts to control some windows from GUIWORLD winpack
# except of Portrait, Options and Dialog windows

###################################################

import GemRB
from GUIDefines import *
import GUICommon
import MessageWindow
import CommonWindow

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContinueWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None

def DialogStarted ():
	global ContinueWindow, OldActionsWindow

	# try to force-close anything which is open
	GUICommon.CloseOtherWindow(None)
	CommonWindow.CloseContainerWindow()

	# we need GUI for dialogs
	GemRB.UnhideGUI()

	# opening control size to maximum, enabling dialog window
	GemRB.GameSetScreenFlags(GS_HIDEGUI, OP_NAND)
	GemRB.GameSetScreenFlags(GS_DIALOG, OP_OR)

	MessageWindow.UpdateControlStatus()

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	ContinueWindow = Window = GemRB.LoadWindow (9)

def DialogEnded ():
	global ContinueWindow, OldActionsWindow

	# TODO: why is this being called at game start?!
	if not ContinueWindow:
		return

	ContinueWindow.Unload ()
	ContinueWindow = None
	OldActionsWindow = None

def CloseContinueWindow ():
	# don't close the actual window now to avoid flickering: we might still want it open
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	if not ContinueWindow:
		return

	ContinueWindow.SetVisible(WINDOW_INVISIBLE)

	MessageWindow.TMessageTA.SetStatus (IE_GUI_CONTROL_FOCUSED)

def OpenEndMessageWindow ():
	ContinueWindow.SetVisible(WINDOW_VISIBLE)
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9371)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

def OpenContinueMessageWindow ():
	ContinueWindow.SetVisible(WINDOW_VISIBLE)
	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9372)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	Button.SetStatus (IE_GUI_CONTROL_FOCUSED)

def OpenReformPartyWindow ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()

	if ReformPartyWindow:
		if ReformPartyWindow:
			ReformPartyWindow.Unload ()
		ReformPartyWindow = None

		GemRB.SetVar ("OtherWindow", -1)
		GemRB.LoadWindowPack ("GUIREC", 800, 600)
		if hideflag:
			GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (42514)
	Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Done
	Button = Window.GetControl (8)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenReformPartyWindow)
	if hideflag:
		GemRB.UnhideGUI ()

def DeathWindowPlot():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	GemRB.HideGUI ()
	GemRB.SetVar("QuitGame1", 32848)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindow():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	GemRB.HideGUI ()
	GemRB.SetVar("QuitGame1", 16498)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindowEnd ():
	GemRB.GamePause (1,3)

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	Window = GemRB.LoadWindow (17)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame1")
	Label.SetText (strref)

	#load
	Button = Window.GetControl (1)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadPress)

	#quit
	Button = Window.GetControl (2)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitPress)

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitPress():
	GemRB.QuitGame()
	GemRB.SetNextScript("Start")
	return

def LoadPress():
	GemRB.QuitGame()
	GemRB.SetNextScript("GUILOAD")
	return

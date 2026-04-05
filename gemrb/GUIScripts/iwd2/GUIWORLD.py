# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later


# GUIW.py - scripts to control some windows from GUIWORLD winpack
# except of Portrait, Options and Dialog windows

###################################################

import GemRB
from GUIDefines import *
import ActionsWindow
import GUICommon
import GUICommonWindows
import MessageWindow
import CommonWindow
import Container

ContinueWindow = None

def OpenDialogButton(id):
	window = GemRB.LoadWindow (id, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	window.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	return window

def DialogStarted ():
	global ContinueWindow

	Container.CloseContainerWindow()

	# opening control size to maximum, enabling dialog window
	CommonWindow.SetGameGUIHidden(False)
	GemRB.GameSetScreenFlags(GS_DIALOG, OP_OR)

	MessageWindow.UpdateControlStatus()

	ContinueWindow = OpenDialogButton(9)

def DialogEnded ():
	global ContinueWindow

	ActionsWindow.UpdateActionsWindow ()

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

def OpenEndMessageWindow ():
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	Button.SetText (9371)
	Button.OnPress (CloseContinueWindow)
	Button.MakeDefault(True)
	ContinueWindow.Focus()

def OpenContinueMessageWindow ():
	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	Button.SetText (9372)
	Button.OnPress (CloseContinueWindow)
	Button.MakeDefault(True)
	ContinueWindow.Focus()

def DeathWindowPlot():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	CommonWindow.SetGameGUIHidden(True)
	GemRB.SetVar("QuitGame1", 32848)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindow():
	#no death movie, but music is changed
	GemRB.LoadMusicPL ("Theme.mus",1)
	CommonWindow.SetGameGUIHidden(True)
	GemRB.SetVar("QuitGame1", 16498)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindowEnd ():
	GemRB.GamePause (1,3)

	Window = GemRB.LoadWindow (17, GUICommon.GetWindowPack())

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame1")
	Label.SetText (strref)

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
	GemRB.QuitGame()
	GemRB.SetNextScript("Start")
	return

def LoadPress():
	GemRB.QuitGame()
	GemRB.SetNextScript("GUILOAD")
	return

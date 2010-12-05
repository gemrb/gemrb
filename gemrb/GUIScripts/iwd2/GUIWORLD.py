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
from GUIClasses import GWindow

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContinueWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None

def CloseContinueWindow ():
	if ContinueWindow:
		# don't close the actual window now to avoid flickering: we might still want it open
		GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	if ContinueWindow:
		ContinueWindow.Unload ()
	GemRB.SetVar ("PortraitWindow", OldActionsWindow.ID)
	ContinueWindow = None
	OldActionsWindow = None
	if hideflag:
		GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GUICommon.GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("PortraitWindow") )
		GemRB.SetVar ("PortraitWindow", Window.ID)

	#end dialog
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9371)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)

	if hideflag:
		GemRB.UnhideGUI ()

def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		GemRB.LoadWindowPack (GUICommon.GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GWindow( GemRB.GetVar ("PortraitWindow") )
		GemRB.SetVar ("PortraitWindow", Window.ID)

	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9372)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)

	if hideflag:
		GemRB.UnhideGUI ()

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
	GemRB.GamePause (1,1)

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

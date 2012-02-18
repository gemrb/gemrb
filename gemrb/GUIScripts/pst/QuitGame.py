# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2007 The GemRB Project
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

# QuitGame.py - display EndGame sequence

###################################################

import GemRB
from GUIDefines import *
import GUICommon

movies = [None,"T1DEATH","T1ABSORB","FINALE"]

def OnLoad ():
	GemRB.HideGUI ()
	which = movies[GemRB.GetVar ("QuitGame1")]
	if which!=None:
		GemRB.PlayMovie (which,1)
	which = movies[GemRB.GetVar ("QuitGame2")]
	if which!=None:
		GemRB.PlayMovie (which,1)
	which = GemRB.GetVar ("QuitGame3")
	if which:
		DeathWindowEnd ()
	else:
		GemRB.QuitGame ()
		GemRB.SetNextScript("Start")

def DonePress ():
		GemRB.QuitGame ()
		GemRB.SetNextScript("Start")

def DeathWindowEnd ():
	GemRB.GamePause (1,3)

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	Window = GemRB.LoadWindow (25)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame3")
	Label.SetText (strref)

	#done
	Button = Window.GetControl (1)
	Button.SetText (17237)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, DonePress)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT|IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.SetVar ("PortraitWindow", Window.ID)
	GemRB.UnhideGUI ()
	#making the playing field gray
	GUICommon.GameWindow.SetVisible(WINDOW_GRAYED)
	return

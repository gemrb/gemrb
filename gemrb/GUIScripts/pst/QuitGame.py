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
import CommonWindow

def OnLoad ():
	movies = ["T1DEATH","T1ABSORB","FINALE"]

	CommonWindow.SetGameGUIHidden(True)
	which = GemRB.GetVar ("QuitGame1")
	if which is not None:
		GemRB.PlayMovie (movies[which], 1)
	which = GemRB.GetVar ("QuitGame2")
	if which is not None:
		GemRB.PlayMovie (movies[which], 1)
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

	Window = GemRB.LoadWindow (25, GUICommon.GetWindowPack())

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	strref = GemRB.GetVar ("QuitGame3")
	Label.SetText (strref)

	#done
	Button = Window.GetControl (1)
	Button.SetText (17237)
	Button.OnPress (DonePress)
	Button.MakeDefault()

	#making the playing field gray
	GUICommon.GameWindow.SetDisabled(True)
	return

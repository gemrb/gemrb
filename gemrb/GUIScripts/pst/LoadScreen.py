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

# LoadScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

LoadScreen = None

LS_TYPE_LOADING = 0
LS_TYPE_SAVING = 1
LS_TYPE_UNLOADING = 2

def StartLoadScreen (screen_type = LS_TYPE_LOADING):
	global LoadScreen

	# While (un)loading, there are no other windows
	if screen_type == LS_TYPE_SAVING:
		GemRB.HideGUI ()
		
	LoadScreen = Window = GemRB.LoadWindow (0, "guils")

	LoadPic = GemRB.GetGameString (STR_LOADMOS)

	if LoadPic == "":
		if screen_type == LS_TYPE_LOADING:
			LoadPic = "GUILS%02d" %GemRB.Roll (1, 20, 0)
		elif screen_type == LS_TYPE_SAVING:
			LoadPic = "GUISG%02d" %GemRB.Roll (1, 10, 0)
		else:
			LoadPic = "GUIDS10"

	Window.SetBackground (LoadPic)
	
	Bar = Window.GetControl (0)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	Bar.SetVarAssoc ("Progress", Progress)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	Skull = Window.GetControl (1)
	Skull.SetMOS ("GSKULOFF")

	if screen_type == LS_TYPE_SAVING:
		GemRB.UnhideGUI ()

	Window.SetVisible (WINDOW_VISIBLE)


def EndLoadScreen ():
	Window = LoadScreen
	Skull = Window.GetControl (1)
	Skull.SetMOS ("GSKULON")
	Window.SetVisible (WINDOW_VISIBLE)
	Window.Unload ()

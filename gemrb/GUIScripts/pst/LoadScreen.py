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
		
	LoadScreen = Window = GemRB.LoadWindow (0, "guils")
	LoadScreen.AddAlias("LOADWIN")

	LoadPic = GemRB.GetGameString (STR_LOADMOS)

	if LoadPic == "":
		if screen_type == LS_TYPE_LOADING:
			LoadPic = "GUILS%02d" %GemRB.Roll (1, 20, 0)
		elif screen_type == LS_TYPE_SAVING:
			LoadPic = "GUISG%02d" %GemRB.Roll (1, 10, 0)
		else:
			LoadPic = "GUIDS10"

	Window.SetBackground (LoadPic)

	def EndLoadScreen ():
		Skull = Window.GetControl (1)
		Skull.SetPicture ("GSKULON")
		
		LoadScreen.OnClose (lambda win: GemRB.GamePause(0, 0))
		GemRB.SetTimer(LoadScreen.Close, 500, 0)
		return
	
	Bar = Window.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.OnEndReached (EndLoadScreen)
	Skull = Window.GetControl (1)
	Skull.SetPicture ("GSKULOFF")

	LoadScreen.ShowModal(MODAL_SHADOW_NONE)

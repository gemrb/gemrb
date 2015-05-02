# -*-python-*-
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

def SetLoadScreen ():
	Table = GemRB.LoadTable ("areaload")
	Area = GemRB.GetGameString (STR_AREANAME)
	LoadPic = Table.GetValue (Area, Table.GetColumnName(0) )
        Middle = LoadScreen.GetControl (4)
        if LoadPic=="*":
		#HoW loadscreens are GTRSK001-GTRSK010
                LoadPic = "GTRSK0"+str(GemRB.Roll(1,10,0)).zfill(2)
        Middle.SetMOS (LoadPic)
	return

def StartLoadScreen ():
	global LoadScreen

	GemRB.LoadWindowPack ("guils", 640, 480)
	LoadScreen = GemRB.LoadWindow (0)

	SetLoadScreen()
	Bar = LoadScreen.GetControl (0)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	Bar.SetVarAssoc ("Progress", Progress)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	LoadScreen.SetVisible (WINDOW_VISIBLE)
	return

def EndLoadScreen ():
	global LoadScreen

	if LoadScreen:
		Skull = LoadScreen.GetControl (3)
		Skull.SetMOS ("GTRBPSK2")
		LoadScreen.SetVisible (WINDOW_VISIBLE)
		LoadScreen.Unload()
	LoadScreen = None
	return

def CloseLoadScreen ():
	global LoadScreen

	if LoadScreen:
		LoadScreen.Unload()
	LoadScreen = None
	return

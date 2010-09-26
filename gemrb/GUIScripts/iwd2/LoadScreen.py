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
import GUICommon
from GUIDefines import *

LoadScreen = None
hide = None

def SetLoadScreen ():
	Table = GemRB.LoadTable ("areaload")
	Area = GemRB.GetGameString (STR_AREANAME)
	LoadPic = Table.GetValue (Area, Table.GetColumnName(0) )

	if LoadPic =="*":
	        LoadPic = GemRB.GetGameString (STR_LOADMOS)

	LoadScreen.SetPicture(LoadPic)
	hide = GemRB.HideGUI()
	if hide:
		GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	return

def StartLoadScreen ():
	global LoadScreen

	GemRB.LoadWindowPack ("guils", 800, 600)
	LoadScreen = GemRB.LoadWindow (0)
	LoadScreen.SetFrame( )

	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic=="":
		LoadPic = "GUILS0"+str(GemRB.Roll(1,9,0))
	LoadScreen.SetPicture(LoadPic)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)

	Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1,tmp,0)
	HintStr = Table.GetValue (tmp, 0)
	TextArea = LoadScreen.GetControl (2)
	TextArea.SetText (HintStr)

	Bar = LoadScreen.GetControl (0)
	Bar.SetVarAssoc ("Progress", Progress)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	LoadScreen.SetVisible (WINDOW_VISIBLE)
	return

def EndLoadScreen ():
	Skull = LoadScreen.GetControl (3)
	Skull.SetMOS ("GTRBPSK2")
	LoadScreen.SetVisible (WINDOW_VISIBLE)
	if hide:
		GemRB.UnhideGUI()
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)

	return

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
import MessageWindow
from GUIDefines import *

LoadScreen = None
Picture = None

def SetLoadScreen ():
	Table = GemRB.LoadTable ("areaload")
	Area = GemRB.GetGameString (STR_AREANAME)
	LoadPic = Table.GetValue (Area, Table.GetColumnName(0) )
	if LoadPic!="*":
		Picture.SetPicture(LoadPic)
	return

def StartLoadScreen ():
	global LoadScreen, Picture

	GemRB.LoadWindowPack ("guils", 800, 600)
	LoadScreen = GemRB.LoadWindow (0)
	LoadScreen.SetFrame( )

	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic=="":
		LoadPic = "GUILS0"+str(GemRB.Roll(1,9,0))
	LoadScreen.SetPicture(LoadPic)
	Progress = GemRB.GetVar ("Progress")

	Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1,tmp,0)
	HintStr = Table.GetValue (tmp, 0)
	
	Label = LoadScreen.GetControl (2)
	Label.SetAlignment(IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE)
	Label.SetText(HintStr)
		
	Picture = LoadScreen.GetControl (4)
	
	def EndLoadScreen ():
		GemRB.SetVar ("Progress", 0)
		MessageWindow.UpdateControlStatus()
		MessageWindow.TMessageTA.Append("[p][color=f1f28d]" + GemRB.GetString (HintStr) + "[/color][/p]\n")

		Skull = LoadScreen.GetControl (3)
		Skull.SetMOS ("GTRBPSK2")
		LoadScreen.SetVisible (WINDOW_VISIBLE)
		LoadScreen.Unload()
		return

	Bar = LoadScreen.GetControl (0)
	Bar.SetVarAssoc ("Progress", Progress)
	Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)
	LoadScreen.SetVisible (WINDOW_VISIBLE)
	return

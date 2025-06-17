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

	LoadScreen = GemRB.LoadWindow (0, "guils")
	LoadScreen.AddAlias("LOADWIN")

	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic != "":
		LoadScreen.SetBackground(LoadPic)

	Table = GemRB.LoadTable ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1, tmp-1, 0)
	HintStr = Table.GetValue (tmp, 0)
	
	Label = LoadScreen.GetControl (2)
	Label.SetText(HintStr)
		
	Picture = LoadScreen.GetControl (4)
	
	def EndLoadScreen ():
		TMessageTA = GemRB.GetView("MsgSys", 0)

		TMessageTA.Append("[p][color=f1f28d]" + GemRB.GetString (HintStr) + "[/color][/p]\n")

		Skull = LoadScreen.GetControl (3)
		Skull.SetPicture ("GTRBPSK2")
		
		LoadScreen.OnClose (lambda win: GemRB.GamePause(0, 0))
		GemRB.SetTimer(LoadScreen.Close, 500, 0)
		return

	Bar = LoadScreen.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.OnEndReached (EndLoadScreen)
	LoadScreen.ShowModal(MODAL_SHADOW_NONE)
	return

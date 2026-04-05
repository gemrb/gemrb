# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

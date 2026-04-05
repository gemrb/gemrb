# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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
	if LoadPic == "*":
		#HoW loadscreens are GTRSK001-GTRSK010
		LoadPic = "GTRSK0"+str(GemRB.Roll (1, 10, 0)).zfill(2)
	Middle.SetPicture (LoadPic)
	return

def StartLoadScreen ():
	global LoadScreen

	LoadScreen = GemRB.LoadWindow (0, "guils")
	LoadScreen.AddAlias("LOADWIN")

	SetLoadScreen()
	Bar = LoadScreen.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.OnEndReached (EndLoadScreen)
	LoadScreen.ShowModal(MODAL_SHADOW_NONE)
	return

def EndLoadScreen ():
	Skull = LoadScreen.GetControl (3)
	Skull.SetPicture ("GTRBPSK2")

	GemRB.SetTimer(LoadScreen.Close, 500, 0)

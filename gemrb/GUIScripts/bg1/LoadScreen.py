# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# LoadScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

LoadScreen = None

def SetLoadScreen ():
	return

def StartLoadScreen ():
	global LoadScreen

	LoadScreen = GemRB.LoadWindow (0, "guils")
	LoadScreen.AddAlias("LOADWIN")

	Middle = LoadScreen.GetControl (4)
	#LoadPic = GemRB.GetGameString (STR_LOADMOS)
	#if LoadPic=="":
	LoadPic = "GTRSK00"+str(GemRB.Roll(1,5,0))
	Middle.SetPicture (LoadPic)
	Bar = LoadScreen.GetControl (0)
	Bar.AddAlias("LOAD_PROG")
	Bar.SetVarAssoc ("Progress", 0)
	Bar.OnEndReached (EndLoadScreen)
	Skull = LoadScreen.GetControl (3)
	Skull.SetPicture ("GTRBPSK")

	LoadScreen.ReparentSubview(Skull, Bar)
	LoadScreen.ShowModal(MODAL_SHADOW_NONE)
	return

def EndLoadScreen ():
	Skull = LoadScreen.GetControl (3)
	Skull.SetPicture ("GTRBPSK2")

	LoadScreen.OnClose (lambda win: GemRB.GamePause(0, 0))
	GemRB.SetTimer(LoadScreen.Close, 500, 0)

	return

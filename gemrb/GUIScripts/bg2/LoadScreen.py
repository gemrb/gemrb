# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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
# $Id$

# LoadScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

LoadScreen = None

def StartLoadScreen ():
	global LoadScreen

	GemRB.LoadWindowPack ("guils", 640, 480)
	LoadScreen = GemRB.LoadWindowObject (0)
	LoadScreen.SetFrame ()
	Middle = LoadScreen.GetControl (3)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic == "":
		LoadPic = "GTRSK00"+str(GemRB.Roll(1,5,1) )
	Middle.SetMOS (LoadPic)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	Table = GemRB.LoadTableObject ("loadhint")
	tmp = Table.GetRowCount ()
	tmp = GemRB.Roll (1,tmp,0)
	HintStr = Table.GetValue (tmp, 0)
	TextArea = LoadScreen.GetControl (2)
	TextArea.SetText (HintStr)
	Bar = LoadScreen.GetControl (0)
	Bar.SetVarAssoc ("Progress", Progress)
	LoadScreen.SetVisible (1)

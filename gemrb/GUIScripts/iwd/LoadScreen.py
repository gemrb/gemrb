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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/LoadScreen.py,v 1.1 2004/09/04 15:55:01 avenger_teambg Exp $

# LoadScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

LoadScreen = None

def StartLoadScreen ():
	global LoadScreen

	GemRB.LoadWindowPack ("guils")
	LoadScreen = GemRB.LoadWindow (0)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	if LoadPic=="":
		LoadPic = "GUILS0"+str(GemRB.Roll(1,9,0))
	GemRB.SetWindowPicture(LoadScreen, LoadPic)
	Bar = GemRB.GetControl (LoadScreen, 0)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	GemRB.SetVarAssoc (LoadScreen, Bar, "Progress", Progress)
	GemRB.SetEvent (LoadScreen, Bar, IE_GUI_PROGRESS_END_REACHED, "EndLoadScreen")
	GemRB.SetVisible (LoadScreen, 1)
	return

def EndLoadScreen ():
	Skull = GemRB.GetControl (LoadScreen, 3)
	GemRB.SetButtonMOS (LoadScreen, Skull, "GTRBPSK2")
	GemRB.SetVisible (LoadScreen, 1)
	return

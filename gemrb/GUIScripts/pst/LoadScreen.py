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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/LoadScreen.py,v 1.4 2005/10/16 21:54:37 edheldil Exp $

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

	# While (un)loading, there are no other windows
	if screen_type == LS_TYPE_SAVING:
		GemRB.HideGUI ()
		
	GemRB.LoadWindowPack ("guils")
	LoadScreen = Window = GemRB.LoadWindow (0)

	LoadPic = GemRB.GetGameString (STR_LOADMOS)

	if LoadPic == "":
		if screen_type == LS_TYPE_LOADING:
			LoadPic = "GUILS%02d" %GemRB.Roll (1, 20, 0)
		elif screen_type == LS_TYPE_SAVING:
			LoadPic = "GUISG%02d" %GemRB.Roll (1, 10, 0)
		else:
			LoadPic = "GUIDS10"

	GemRB.SetWindowPicture (Window, LoadPic)
	
	Bar = GemRB.GetControl (Window, 0)
	Progress = 0
	GemRB.SetVar ("Progress", Progress)
	GemRB.SetVarAssoc (Window, Bar, "Progress", Progress)
	GemRB.SetEvent (Window, Bar, IE_GUI_PROGRESS_END_REACHED, "EndLoadScreen")
	Skull = GemRB.GetControl (Window, 1)
	GemRB.SetButtonMOS (Window, Skull, "GSKULOFF")

	if screen_type == LS_TYPE_SAVING:
		GemRB.UnhideGUI ()

	GemRB.SetVisible (Window, 1)


def EndLoadScreen ():
	Window = LoadScreen
	Skull = GemRB.GetControl (Window, 1)
	GemRB.SetButtonMOS (Window, Skull, "GSKULON")
	GemRB.SetVisible (Window, 1)

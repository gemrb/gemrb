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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIMA.py,v 1.1 2004/01/11 16:49:09 edheldil Exp $


# GUIMA.py - scripts to control map windows from GUIMA winpack

###################################################

import GemRB
from GUIDefines import *

from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
import GUICommonWindows

def OpenMapWindow ():
	global MainWindow

	CloseCommonWindows ()

	GemRB.LoadWindowPack ("GUIMA")
        OpenCommonWindows ()
	#MainWindow = Window = GemRB.LoadWindow (3)
	MainWindow = Window = GUICommonWindows.MainWindow

	# World Map
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20429)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")

	# Add Note
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4182)

	# 2 - map name?
	# 3 - map bitmap?
	# 4 - ???
	
	# Done
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 1403)

	# FIXME: should be hidden while configured and only now made visible?
	GemRB.SetVisible (Window, 1)

def OpenWorldMapWindow ():
	global MainWindow, WorldMapWindow

	CloseCommonWindows ()

	GemRB.LoadWindowPack ("GUIWMAP")
	WorldMapWindow = GemRB.LoadWindow (0)
	#GUICommonWindows.MainWindow = WorldMapWindow

	# Done
	Button = GemRB.GetControl (WorldMapWindow, 0)
	GemRB.SetText (WorldMapWindow, Button, 1403)
	GemRB.SetEvent (WorldMapWindow, Button, IE_GUI_BUTTON_ON_PRESS, "CloseWorldMapWindow")
	GemRB.SetVisible (WorldMapWindow, 1)

def CloseWorldMapWindow ():
	GemRB.UnloadWindow (WorldMapWindow)
	OpenMapWindow ()


###################################################
# End of file GUIMA.py

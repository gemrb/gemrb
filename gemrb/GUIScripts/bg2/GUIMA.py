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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg2/GUIMA.py,v 1.2 2004/08/27 13:36:31 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *

#from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
#import GUICommonWindows

MapWindow = None
WorldMapWindow = None

###################################################
def OpenMapWindow ():
	global MapWindow

	GemRB.HideGUI()

	if MapWindow:
		if WorldMapWindow: OpenWorldMapWindow ()
		
		GemRB.UnloadWindow (MapWindow)
		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIMAP")
	MapWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MapWindow)

	# World Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindowInside")

	# Hide or Show mapnotes
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "RefreshNoteWindow")
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetVarAssoc (Window, Button, "ShowMapNotes", 1)

	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetControlSize (Window, Label, 0, 0)
	GemRB.SetControlPos (Window, Label, 65535, 0) #remove this crap

	# Map Control
	GemRB.CreateMapControl (Window, 2, 24, 23, 480, 360)
	Map = GemRB.GetControl (Window, 2)

	#GemRB.SetEvent (Window, Map, IE_GUI_MAPWINDOW_ON_PRESS, "LeftPressMap")
	#GemRB.SetEvent (Window, Map, IE_GUI_MAPWINDOW_ON_DOUBLEPRESS, "LeftPressMap")
	#GemRB.SetEvent (Window, Map, IE_GUI_MAPWINDOW_ON_RIGHTPRESS, "AddNoteWindow")

	GemRB.UnhideGUI ()

def LeftDoublePressMap ():
	print "MoveToPoint"
	return

def LeftPressMap ():
	print "MoveRectangle"
	return

def AddNoteWindow ():
	print "Add Note"
	return

def OpenWorldMapWindowInside ():
	WorldMapWindowCommon(0)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon(1)
	return

def WorldMapWindowCommon(Travel):
	global WorldMapWindow

	GemRB.HideGUI()

	if WorldMapWindow:
		GemRB.UnloadWindow (WorldMapWindow)
		WorldMapWindow = None
		GemRB.SetVar ("OtherWindow", MapWindow)

		GemRB.UnhideGUI ()
		return

	GemRB.LoadWindowPack ("GUIWMAP")
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", WorldMapWindow)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel)
	Button = GemRB.GetControl (Window, 4)
	
	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	if Travel:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")
	else:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")
	GemRB.UnhideGUI ()


###################################################
# End of file GUIMA.py

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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg1/GUIMA.py,v 1.3 2006/01/03 17:16:04 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

MapWindow = None
WorldMapWindow = None
WorldMapControl = None

###################################################
def OpenMapWindow ():
	global MapWindow

	if CloseOtherWindow (OpenMapWindow):
		if WorldMapWindow: OpenWorldMapWindowInside ()

		GemRB.HideGUI ()
		GemRB.UnloadWindow (MapWindow)
		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIMAP")
	MapWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MapWindow)
	SetupMenuWindowControls (GUICommonWindows.OptionsWindow, 0, "OpenMapWindow")

	# World Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindowInside")

	# Map Control
	GemRB.CreateMapControl (Window, 2, 0, 0, 0, 0)
	Map = GemRB.GetControl (Window, 2)

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
	WorldMapWindowCommon (-1)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def MoveToNewArea ():
	global WorldMapWindow, WorldMapControl

	tmp = GemRB.GetDestinationArea (WorldMapWindow, WorldMapControl)
	print tmp
	CloseWorldMapWindow ()
	GemRB.CreateMovement (tmp["Destination"], tmp["Entrance"])
	return


def ChangeTooltip ():
	global WorldMapWindow, WorldMapControl
	global str

	tmp = GemRB.GetDestinationArea (WorldMapWindow, WorldMapControl)
	print tmp
	if (tmp):
		str = "%s: %d"%(GemRB.GetString(23084),tmp["Distance"])
	else:
		str=""

	GemRB.SetTooltip (WorldMapWindow, WorldMapControl, str)
	return


def CloseWorldMapWindow():
	global WorldMapWindow, WorldMapControl

	GemRB.UnloadWindow (WorldMapWindow)
	WorldMapWindow = None
	WorldMapControl = None
	GemRB.SetVisible (0,1)
	GemRB.UnhideGUI ()
	return


def WorldMapWindowCommon (Travel):
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		CloseWorldMapWindow ()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIWMAP")
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	MapWindow = None

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel, "floattxt")
	WorldMapControl = GemRB.GetControl (Window, 4)
	GemRB.SetAnimation (Window, WorldMapControl, "WMDAG")

	#north
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapN")

	#south
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapS")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseWorldMapWindow")
	GemRB.SetVisible (Window, 1)
	return

def MapN():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 0, -10)
	return

def MapS():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 0, 10)
	return


###################################################
# End of file GUIMA.py

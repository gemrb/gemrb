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
# $Id$


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow

MapWindow = None
WorldMapWindow = None
WorldMapControl = None

def RevealMap ():
	global MapWindow

	if CloseOtherWindow (ShowMap):
		GemRB.UnloadWindow (MapWindow)

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()

	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")

	GemRB.RevealArea (PosX, PosY, 30, 1)
	GemRB.GamePause (0,0)
	return
###################################################
# for farsight effect
###################################################
def ShowMap ():
	global MapWindow

	if CloseOtherWindow (ShowMap):
		GemRB.UnloadWindow (MapWindow)

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIMAP", 640, 480)
	MapWindow = Window = GemRB.LoadWindow (2)
	#this window type blocks the game normally, but map window doesn't
	GemRB.SetVar ("OtherWindow", MapWindow)
	#saving the original portrait window
	GemRB.SetWindowFrame (MapWindow)

	# World Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	# Hide or Show mapnotes
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetButtonState (Window, Button, IE_GUI_BUTTON_LOCKED)

	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, "")
	# Map Control
	GemRB.CreateMapControl (Window, 2, 0, 0, 0, 0)
	Map = GemRB.GetControl (Window, 2)
	GemRB.SetVar("ShowMapNotes",IE_GUI_MAP_REVEAL_MAP)
	GemRB.SetVarAssoc (Window, Map, "ShowMapNotes", IE_GUI_MAP_REVEAL_MAP)
	GemRB.SetEvent (Window, Map, IE_GUI_MAP_ON_PRESS, "RevealMap")
	GemRB.SetVisible (Window, 1)
	GemRB.GamePause (0,0)
	return

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
	GemRB.LoadWindowPack ("GUIMAP", 640, 480)
	MapWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MapWindow)

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

def CloseWorldMapWindow ():
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

	GemRB.LoadWindowPack ("GUIWMAP", 640, 480)
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	MapWindow = None
	GemRB.SetVar ("OtherWindow", WorldMapWindow)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel, "infofont")
	WorldMapControl = GemRB.GetControl (Window, 4)
	GemRB.SetAnimation (Window, WorldMapControl, "WMDAG")
	GemRB.SetEvent (Window, WorldMapControl, IE_GUI_WORLDMAP_ON_PRESS, "MoveToNewArea")
	GemRB.SetEvent (Window, WorldMapControl, IE_GUI_MOUSE_ENTER_WORLDMAP, "ChangeTooltip")

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

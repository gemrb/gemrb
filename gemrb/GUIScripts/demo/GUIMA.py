# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2020 The GemRB Project
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

# GUIMA.py - scripts to control map windows from the GUIMA and GUIWMAP winpacks

###################################################

import GemRB
import GUICommon
import GUICommonWindows
import GUIMACommon
from GUIDefines import *

MapWindow = None
WorldMapWindow = None
WorldMapControl = None

WIDTH = 800
HEIGHT = 600

def OpenMapWindow ():
	global MapWindow

	if GUICommon.CloseOtherWindow (OpenMapWindow):
		if MapWindow:
			MapWindow.Unload ()

		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIMAP", WIDTH, HEIGHT)
	MapWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", MapWindow.ID)

	Label = Window.GetControl (0)
	Label.SetText ("Area map")

	# World Map
	Button = Window.GetControl (1)
	Button.SetText ("WMAP")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenWorldMapWindowInside)

	# Map Control
	Window.CreateMapControl (2, 0, 0, 0, 0)
	Map = Window.GetControl (2)
	Map.SetEvent (IE_GUI_MAP_ON_DOUBLE_PRESS, LeftDoublePressMap)

	Button = Window.GetControl (3)
	Button.SetText ("Close")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMapWindow)

	Window.SetVisible (WINDOW_VISIBLE)
	Map.SetStatus (IE_GUI_CONTROL_FOCUSED)

	# create a button so we can map it do ESC for quit exiting
	Button = Window.CreateButton (99, 0, 0, 1, 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMapWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GUICommonWindows.SetSelectionChangeHandler(None)
	return

def LeftDoublePressMap ():
	# close the map on doubleclick
	OpenMapWindow()
	return

def OpenWorldMapWindowInside ():
	global MapWindow

	OpenMapWindow () #closes mapwindow
	MapWindow = -1
	WorldMapWindowCommon (-1)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def ChangeTooltip ():
	global WorldMapWindow, WorldMapControl

	tt = ""
	area = WorldMapControl.GetDestinationArea ()
	if area and area["Distance"] >= 0:
		str = GemRB.GetString(23084)
		if (str):
			tt = "%s: %d"%(str,area["Distance"])

	WorldMapControl.SetTooltip (tt)
	return

def CloseWorldMapWindow ():
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		WorldMapWindow.Unload ()
	WorldMapWindow = None
	WorldMapControl = None
	GemRB.SetVar ("OtherWindow", -1)
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
	GemRB.UnhideGUI ()
	return

def WorldMapWindowCommon (Travel):
	global WorldMapWindow, WorldMapControl

	if GUICommon.CloseOtherWindow (CloseWorldMapWindow):
		CloseWorldMapWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIMAP", WIDTH, HEIGHT)
	WorldMapWindow = Window = GemRB.LoadWindow (0)

	Label = Window.GetControl (0)
	Label.SetText ("World map")

	Button = Window.GetControl (1)
	Button.SetText ("MAP")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMapWindow)

	GemRB.SetVar ("OtherWindow", WorldMapWindow.ID)
	Window.SetFrame ()

	Window.CreateWorldMapControl (2, 0, 0, 0, 0, Travel, "floattxt")
	WorldMapControl = Window.GetControl (2)
	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, GUIMACommon.MoveToNewArea)
	WorldMapControl.SetEvent (IE_GUI_MOUSE_ENTER_WORLDMAP, ChangeTooltip)
	# center on current area
	MapC()

	Button = Window.GetControl (3)
	Button.SetText ("Close")
	if Travel >= 0:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenWorldMapWindow)
	else:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMapWindow) # TODO: change to CloseTopWindow
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	Window.SetVisible (WINDOW_VISIBLE)
	WorldMapControl.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def MapC():
	WorldMapControl.AdjustScrolling (0, 0)
	return

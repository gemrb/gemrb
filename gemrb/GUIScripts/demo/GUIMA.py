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
import GUICommonWindows
import GUIMACommon
from GUIDefines import *

MapWindow = None
WorldMapControl = None

def InitMapWindow (Window):
	global MapWindow
	MapWindow = Window

	Label = Window.GetControl (0)
	Label.SetText ("Area map")

	# World Map
	Button = Window.GetControl (1)
	Button.SetText ("WMAP")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenWorldMapWindow())

	# Map Control
	Map = Window.ReplaceSubview (2, IE_GUI_MAP)
	Map.SetAction (lambda: Window.Close(), IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)

	Button = Window.GetControl (3)
	Button.SetText ("Close")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.CloseTopWindow)

	Map.Focus ()
	return

def InitWorldMapWindow (Window):
	WorldMapWindowCommon (Window, -1)
	return

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader (0, "GUIMAP", GUICommonWindows.ToggleWindow, InitMapWindow)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader (0, "GUIMAP", GUICommonWindows.OpenWindowOnce, InitMapWindow)
OpenWorldMapWindow = GUICommonWindows.CreateTopWinLoader (0, "GUIMAP", GUICommonWindows.OpenWindowOnce, InitWorldMapWindow)

def OpenTravelWindow ():
	Window = OpenWorldMapWindow ()
	WorldMapWindowCommon (Window, GemRB.GetVar ("Travel"))
	return

def ChangeTooltip ():
	global WorldMapControl

	tt = ""
	area = WorldMapControl.GetDestinationArea ()
	if area and area["Distance"] >= 0:
		str = GemRB.GetString(23084)
		if (str):
			tt = "%s: %d"%(str,area["Distance"])

	WorldMapControl.SetTooltip (tt)
	return

def WorldMapWindowCommon (Window, Travel):
	global WorldMapControl

	Label = Window.GetControl (0)
	Label.SetText ("World map")

	Button = Window.GetControl (1)
	Button.SetText ("MAP")
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenMapWindow ())

	WorldMapControl = Window.ReplaceSubview (2, IE_GUI_WORLDMAP, Travel, "floattxt")
	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, GUIMACommon.MoveToNewArea)
	WorldMapControl.SetAction (ChangeTooltip, IE_ACT_MOUSE_ENTER)
	# center on current area
	MapC()

	Button = Window.GetControl (3)
	Button.SetText ("Close")
	if Travel >= 0:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenMapWindow ())
	else:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.CloseTopWindow)
	Button.SetHotKey ('m')

	Window.SetVisible (WINDOW_VISIBLE)
	WorldMapControl.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def MapC():
	WorldMapControl.AdjustScrolling (0, 0)
	return

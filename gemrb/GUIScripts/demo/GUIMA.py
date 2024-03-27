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

WorldMapControl = None
AreaMapControl = None

def InitMapWindow (Window, WorldMap = False):
	global WorldMapControl, AreaMapControl

	Label = Window.GetControl (0)
	if WorldMap:
		Label.SetText ("World map")
	else:
		Label.SetText ("Area map")

	# World Map
	Button = Window.GetControl (1)
	if WorldMap:
		Button.SetText ("MAP")
	else:
		Button.SetText ("WMAP")
	Button.OnPress (lambda: InitMapWindow (Window, not WorldMap))

	# Map or World Map control
	if WorldMap:
		if AreaMapControl:
			AreaMapControl.SetVisible (False)
			AreaMapControl.SetDisabled (True)

		if WorldMapControl:
			WorldMapControl.SetVisible (True)
			WorldMapControl.SetDisabled (False)
		else:
			WorldMapControl = Window.ReplaceSubview (2, IE_GUI_WORLDMAP, "floattxt", "WMDAG")
			WorldMapControl.OnPress (GUIMACommon.MoveToNewArea)

		# center on current area
		WorldMapControl.Scroll (0, 0, False)
		WorldMapControl.Focus ()
	else:
		if WorldMapControl:
			WorldMapControl.SetVisible (False)
			WorldMapControl.SetDisabled (True)

		if AreaMapControl:
			AreaMapControl.SetVisible (True)
			AreaMapControl.SetDisabled (False)
		else:
			AreaMapControl = Window.ReplaceSubview (4, IE_GUI_MAP)
			AreaMapControl.SetAction (CloseMapWindow, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)
		AreaMapControl.Focus ()

	Button = Window.GetControl (3)
	Button.SetText ("Close")
	Button.OnPress (CloseMapWindow)
	Button.SetHotKey ('m')

	# workaround for proper closure with ESC
	Button = Window.GetControl (99)
	if not Button:
		Button = Window.CreateButton (99, 0, 0, 0, 0)
	Button.OnPress (CloseMapWindow)
	Button.MakeEscape ()

	return

def CloseMapWindow ():
	global WorldMapControl, AreaMapControl

	WorldMapControl = None
	AreaMapControl = None
	GUICommonWindows.CloseTopWindow ()

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader (0, "GUIMAP", GUICommonWindows.ToggleWindow, InitMapWindow)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader (0, "GUIMAP", GUICommonWindows.OpenWindowOnce, InitMapWindow)

def OpenTravelWindow (Travel):
	Window = OpenMapWindow ()
	InitMapWindow (Window, True)
	WorldMapControl.SetVarAssoc("Travel", Travel)
	return

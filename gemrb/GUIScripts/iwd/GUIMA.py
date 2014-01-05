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


# GUIMA.py - scripts to control map windows from the GUIMA and GUIWMAP winpacks

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
from GUIDefines import *

MapWindow = None
NoteWindow = None
WorldMapWindow = None
WorldMapControl = None
PortraitWindow = None
OldPortraitWindow = None
OptionsWindow = None
OldOptionsWindow = None

def RevealMap ():
	global MapWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (ShowMap):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None

	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")

	GemRB.RevealArea (PosX, PosY, 30, 1)
	GemRB.GamePause (0,0)
	return

###################################################
# for farsight effect
###################################################
def ShowMap ():
	import GameCheck
	import GUICommonWindows
	global MapWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (ShowMap):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIMAP", 640, 480)
	MapWindow = Window = GemRB.LoadWindow (2)
	#this window type blocks the game normally, but map window doesn't
	GemRB.SetVar ("OtherWindow", MapWindow.ID)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, ShowMap)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)
	OptionsWindow.SetFrame ()

	# World Map
	Button = Window.GetControl (1)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	# Hide or Show mapnotes
	Button = Window.GetControl (3)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Label = Window.GetControl (0x10000003)
	Label.SetText ("")

	# Map Control
	if GameCheck.IsBG2():
		Window.CreateMapControl (2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	else:
		Window.CreateMapControl (2, 0, 0, 0, 0)
	Map = Window.GetControl (2)
	GemRB.SetVar ("ShowMapNotes",IE_GUI_MAP_REVEAL_MAP)
	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_REVEAL_MAP)
	Map.SetEvent (IE_GUI_MAP_ON_PRESS, RevealMap)
	Window.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_GRAYED)
	PortraitWindow.SetVisible (WINDOW_GRAYED)
	OptionsWindow.SetVisible (WINDOW_FRONT)
	PortraitWindow.SetVisible (WINDOW_FRONT)
	Window.SetVisible (WINDOW_FRONT)
	# TODO: probably more of this function needs ifdefing
	if HasMapNotes ():
		Map.SetStatus (IE_GUI_CONTROL_FOCUSED| IE_GUI_MAP_REVEAL_MAP)
	else:
		Map.SetStatus (IE_GUI_CONTROL_FOCUSED)
	GemRB.GamePause (0,0)
	return

###################################################
def OpenMapWindow ():
	global MapWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GUICommon.CloseOtherWindow (OpenMapWindow):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIMAP", 640, 480)
	MapWindow = Window = GemRB.LoadWindow (2)
	#this window type blocks the game normally, but map window doesn't
	GemRB.SetVar ("OtherWindow", MapWindow.ID)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	if GameCheck.IsBG2():
		GUICommonWindows.MarkMenuButton (OptionsWindow)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenMapWindow)
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow (0)
	OptionsWindow.SetFrame ()

	# World Map
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenWorldMapWindowInside)

	# Hide or Show mapnotes
	if HasMapNotes ():
		Button = Window.GetControl (3)
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		# Is this an option?
		GemRB.SetVar ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
		Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

		Label = Window.GetControl (0x10000003)
		Label.SetText ("")

	# Map Control
	if GameCheck.IsBG2():
		Window.CreateMapControl (2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	else:
		Window.CreateMapControl (2, 0, 0, 0, 0)
	Map = Window.GetControl (2)
	if HasMapNotes ():
		Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
		Map.SetEvent (IE_GUI_MAP_ON_RIGHT_PRESS, AddNoteWindow)
	Map.SetEvent (IE_GUI_MAP_ON_DOUBLE_PRESS, LeftDoublePressMap)

	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	Window.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	if HasMapNotes ():
		Map.SetStatus (IE_GUI_CONTROL_FOCUSED | IE_GUI_MAP_VIEW_NOTES)
	else:
		Map.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def HasMapNotes ():
	return GameCheck.IsBG2() or GameCheck.IsIWD2() or GameCheck.IsPST()

def LeftDoublePressMap ():
	#close the map on doubleclick
	OpenMapWindow()
	return

def OpenWorldMapWindowInside ():
	global MapWindow

	OpenMapWindow () #closes mapwindow
	MapWindow = -1
	print "MapWindow=",MapWindow
	WorldMapWindowCommon (-1)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def MoveToNewArea ():
	global WorldMapWindow, WorldMapControl

	tmp = WorldMapControl.GetDestinationArea (1)
	CloseWorldMapWindow ()
	
	if tmp["Destination"].lower() == GemRB.GetGameString(STR_AREANAME).lower():
		return
	elif tmp["Distance"] == -1:
		print "Invalid target", tmp
		return

	GemRB.CreateMovement (tmp["Destination"], tmp["Entrance"], tmp["Direction"])
	# distance is stored in hours, but the action needs seconds
	GemRB.ExecuteString ("AdvanceTime(%d)"%(tmp["Distance"]*300))
	return

def ChangeTooltip ():
	global WorldMapWindow, WorldMapControl
	global str

	tmp = WorldMapControl.GetDestinationArea ()
	if tmp and tmp["Distance"] >= 0:
		str = "%s: %d"%(GemRB.GetString(23084),tmp["Distance"])
	else:
		str=""

	WorldMapControl.SetTooltip (str)
	return

def CloseWorldMapWindow ():
	global WorldMapWindow, WorldMapControl

	print "CloseWorldMapWindow found Mapwindow = ",MapWindow
	if MapWindow:
		# reopen map window
		if WorldMapWindow:
			WorldMapWindow.Unload ()
		WorldMapWindow = None
		WorldMapControl = None
		OpenMapWindow ()
		return

	if WorldMapWindow:
		WorldMapWindow.Unload ()
	WorldMapWindow = None
	WorldMapControl = None
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
	GemRB.UnhideGUI ()
	return

def WorldMapWindowCommon (Travel):
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		CloseWorldMapWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIWMAP", 640, 480)
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	#saving the original portrait window
	Window.SetFrame ()

	if GameCheck.IsBG2():
		Window.CreateWorldMapControl (4, 0, 62, 640, 418, Travel, "floattxt")
	else:
		Window.CreateWorldMapControl (4, 0, 62, 640, 418, Travel, "infofont")
	WorldMapControl = Window.GetControl (4)
	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, MoveToNewArea)
	WorldMapControl.SetEvent (IE_GUI_MOUSE_ENTER_WORLDMAP, ChangeTooltip)
	#center on current area
	WorldMapControl.AdjustScrolling (0,0)

	#north
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapN)

	#south
	Button = Window.GetControl (2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapS)

	# Done
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseWorldMapWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	Window.SetVisible (WINDOW_VISIBLE)
	WorldMapControl.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def MapN():
	WorldMapControl.AdjustScrolling (0, -10)
	return

def MapS():
	WorldMapControl.AdjustScrolling (0, 10)
	return


###################################################
# End of file GUIMA.py

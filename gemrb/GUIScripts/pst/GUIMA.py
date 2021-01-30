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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
import GUICommon
import GUICommonWindows
import GUIMACommon
from GUIDefines import *

WorldMapControl = None

###################################################
def InitMapWindow (Window):
	Window.AddAlias("WIN_MAP")

	# World Map
	Button = Window.GetControl (0)
	Button.SetText (20429)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenTravelWindow())

	# Add Note
	Button = Window.GetControl (1)
	Button.SetText (4182)
	Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_SET_NOTE)

	# Note text
	Edit = Window.GetControl (4)
	Edit.SetEvent (IE_GUI_EDIT_ON_DONE, NoteChanged)
	Edit.SetFlags(IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	# Map Control
	# ronote and usernote are the pins for the notes
	Map = Window.ReplaceSubview(3, IE_GUI_MAP, Edit)
	Map.AddAlias("MAP")

	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	Map.SetStatus (IE_GUI_MAP_VIEW_NOTES)

	Map.SetEvent (IE_GUI_MAP_ON_PRESS, SetMapNote)
	Map.SetAction(Window.Close, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)

	MapTable = GemRB.LoadTable( "MAPNAME" )
	MapName = MapTable.GetValue (GemRB.GetCurrentArea (), "STRING")
	
	Label = Window.GetControl (0x10000005)
	Label.SetText (MapName)
	#Label.SetTextColor (255, 0, 0)
	# 2 - map name?
	# 3 - map bitmap?
	# 4 - ???

	# these overlap the map. lets move them to be children of the map
	Label = Window.ReparentSubview(Label, Map)
	Edit = Window.ReparentSubview(Edit, Map)

	# Done
	Button = Window.GetControl (5)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())
	Button.MakeEscape()

	return

def OpenTravelWindow ():
	global WorldMapControl

	GUICommonWindows.DisableAnimatedWindows ()

	Travel = GemRB.GetVar ("Travel")

	Window = GemRB.LoadWindow (0, "GUIWMAP")

	WorldMapControl = WMap = Window.ReplaceSubview (4, IE_GUI_WORLDMAP, "FONTDLG")
	WMap.SetTextColor (IE_GUI_WMAP_COLOR_BACKGROUND, {'r' : 0x84, 'g' : 0x4a, 'b' : 0x2c, 'a' : 0x00})
	WMap.SetTextColor (IE_GUI_WMAP_COLOR_NORMAL, {'r' : 0x20, 'g' : 0x20, 'b' : 0x00, 'a' : 0xff})
	WMap.SetTextColor (IE_GUI_WMAP_COLOR_SELECTED, {'r' : 0x20, 'g' : 0x20, 'b' : 0x00, 'a' : 0xff})
	WMap.SetTextColor (IE_GUI_WMAP_COLOR_NOTVISITED, {'r' : 0x20, 'g' : 0x20, 'b' : 0x00, 'a' : 0xa0})
	WMap.SetAnimation ("WMPTY")
	WMap.SetVarAssoc("Travel", GemRB.GetVar("Travel"))
	#center on current area
	WMap.Scroll (0,0)
	WMap.Focus()
	if Travel:
		WMap.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, GUIMACommon.MoveToNewArea)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: Window.Close())

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMA", GUICommonWindows.ToggleWindow, InitMapWindow, None, WINDOW_TOP|WINDOW_HCENTER)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMA", GUICommonWindows.OpenWindowOnce, InitMapWindow, None, WINDOW_TOP|WINDOW_HCENTER)

def NoteChanged (Edit):
	Edit.SetFlags(IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	Text = Edit.QueryText ()
	Edit.SetText ("")
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	GemRB.SetMapnote (PosX, PosY, 1, Text)

	Map = GemRB.GetView("MAP")
	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	Map.SetStatus (IE_GUI_MAP_VIEW_NOTES)

	return
	
def SetMapNote (Map):
	if GemRB.GetVar ("ShowMapNotes") != IE_GUI_MAP_SET_NOTE:
		return

	MapWindow = GemRB.GetView("WIN_MAP")

	Edit = MapWindow.GetControl (4)
	Edit.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_NAND)
	Edit.Focus()

	return

###################################################
# End of file GUIMA.py


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
	Button.OnPress (OpenTravelWindow)

	# Add Note
	Button = Window.GetControl (1)
	Button.SetText (4182)
	Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_SET_NOTE)

	# Note text
	Edit = Window.GetControl (4)
	Edit.OnDone (NoteChanged)
	Edit.SetFlags(IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	# Map Control
	# ronote and usernote are the pins for the notes
	Map = Window.ReplaceSubview(3, IE_GUI_MAP, Edit)
	Map.AddAlias("MAP")

	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	Map.SetStatus (IE_GUI_MAP_VIEW_NOTES)

	Map.OnPress (SetMapNote)
	Map.SetAction(Window.Close, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)

	MapTable = GemRB.LoadTable( "MAPNAME" )
	MapName = MapTable.GetValue (GemRB.GetCurrentArea (), "STRING")
	
	Label = Window.GetControl (0x10000005)
	Label.SetText (MapName)
	#Label.SetColor (255, 0, 0)
	# 2 - map name?
	# 3 - map bitmap?
	# 4 - ???

	# these overlap the map. lets move them to be children of the map
	Label = Window.ReparentSubview(Label, Map)
	Edit = Window.ReparentSubview(Edit, Map)

	# Done
	Button = Window.GetControl (5)
	Button.SetText (1403)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	return

def OpenTravelWindow ():
	global WorldMapControl

	Travel = GemRB.GetVar ("Travel")

	Window = GemRB.LoadWindow (0, "GUIWMAP")
	Window.AddAlias ("WIN_PSTWMAP")

	color = {'r' : 30, 'g' : 8, 'b' : 0, 'a' : 0xff}
	WorldMapControl = WMap = Window.ReplaceSubview (4, IE_GUI_WORLDMAP, "FONTDLG", "WMPTY", color, color, color)
	WMap.SetVarAssoc ("Travel", Travel)
	#center on current area
	WMap.Scroll (0,0)
	WMap.Focus()
	if Travel != -1:
		WMap.OnPress (GUIMACommon.MoveToNewArea)

	# Done
	Button = Window.GetControl (0)
	Button.SetText (1403)
	Button.OnPress (Window.Close)

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMA", GUICommonWindows.ToggleWindow, InitMapWindow)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader(3, "GUIMA", GUICommonWindows.OpenWindowOnce, InitMapWindow)

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


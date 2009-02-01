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
# $Id$


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import EnableAnimatedWindows, DisableAnimatedWindows

MapWindow = None
WorldMapWindow = None
PosX = 0
PosY = 0

###################################################
def OpenMapWindow ():
	global MapWindow

	if CloseOtherWindow (OpenMapWindow):
		GemRB.HideGUI ()
		if WorldMapWindow: OpenWorldMapWindowInside ()
		
		GemRB.UnloadWindow (MapWindow)
		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIMA")
	MapWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("OtherWindow", MapWindow)

	# World Map
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 20429)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")

	# Add Note
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4182)
	GemRB.SetVarAssoc (Window, Button, "x", IE_GUI_MAP_SET_NOTE)

	# Note text
	Text = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Text, "")

	Edit = GemRB.CreateTextEdit (Window, 6, 0, 0, 200, 100, "FONTDLG", "pokus...")
	#GemRB.SetControlPos (65535, 65535)

	# Map Control
	# ronote and usernote are the pins for the notes
	# 4 is the Label's control ID
	GemRB.CreateMapControl (Window, 3, 24, 23, 480, 360, 4, "USERNOTE","RONOTE")
	Map = GemRB.GetControl (Window, 3)
	GemRB.SetVar ("x",IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetVarAssoc (Window, Map, "x", IE_GUI_MAP_VIEW_NOTES)

	GemRB.SetEvent (Window, Map, IE_GUI_MAP_ON_PRESS, "SetMapNote")

	MapTable = GemRB.LoadTable( "MAPNAME" )
	MapName = GemRB.GetTableValue (MapTable, GemRB.GetCurrentArea (), "STRING")
	GemRB.UnloadTable( MapTable )
	
	Label = GemRB.GetControl (Window, 0x10000005)
	GemRB.SetText (Window, Label, MapName)
	#GemRB.SetLabelTextColor (Window, Label, 255, 0, 0)
	# 2 - map name?
	# 3 - map bitmap?
	# 4 - ???

	# Done
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")

	GemRB.UnhideGUI ()

def NoteChanged ():
	Label = GemRB.GetControl (MapWindow, 6)
	Text = GemRB.QueryText (MapWindow, Label)
	GemRB.SetMapnote (PosX, PosY, 0, Text)
	return
	
def SetMapNote ():
	global PosX, PosY

	Label = GemRB.GetControl (MapWindow, 6)
	GemRB.SetEvent (MapWindow, Label, IE_GUI_EDIT_ON_CHANGE, "NoteChanged")
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	GemRB.SetControlStatus (MapWindow, Label, IE_GUI_CONTROL_FOCUSED)
	Map = GemRB.GetControl (MapWindow, 3)
	GemRB.SetVar ("x",IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetVarAssoc (MapWindow, Map, "x", IE_GUI_MAP_VIEW_NOTES)
	return

def OpenWorldMapWindowInside ():
	WorldMapWindowCommon (-1)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def WorldMapWindowCommon (Travel):
	global WorldMapWindow

	GemRB.HideGUI()

	if WorldMapWindow:
		GemRB.UnloadWindow (WorldMapWindow)
		WorldMapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		EnableAnimatedWindows ()
		GemRB.UnhideGUI ()
		return

	DisableAnimatedWindows ()
	GemRB.LoadWindowPack ("GUIWMAP")
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	MapWindow = None
	GemRB.SetVar ("OtherWindow", WorldMapWindow)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel, "FONTDLG")
	WMap = GemRB.GetControl (Window, 4)
	GemRB.SetWorldMapTextColor (Window, WMap, IE_GUI_WMAP_COLOR_NORMAL, 0x20, 0x20, 0x00, 0xff)
	GemRB.SetWorldMapTextColor (Window, WMap, IE_GUI_WMAP_COLOR_SELECTED, 0x20, 0x20, 0x00, 0xff)
	GemRB.SetWorldMapTextColor (Window, WMap, IE_GUI_WMAP_COLOR_NOTVISITED, 0x20, 0x20, 0x00, 0xa0)
	GemRB.SetAnimation (Window, WMap, "WMPTY")
	
	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	if Travel>=0:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")
	else:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")
	GemRB.UnhideGUI ()

###################################################
# End of file GUIMA.py


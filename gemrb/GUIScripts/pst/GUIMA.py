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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIMA.py,v 1.16 2004/11/21 21:28:22 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow

MapWindow = None
WorldMapWindow = None
PosX = 0
PosY = 0

###################################################
def OpenMapWindow ():
	global MapWindow

	if CloseOtherWindow (OpenMapWindow):
		GemRB.HideGUI ()
		if WorldMapWindow: OpenWorldMapWindow ()
		
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
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "AddMapNote")

	Text = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Text, "")

	# Map Control
	# ronote and usernote are the pins for the notes
	# 4 is the Label's control ID
	GemRB.CreateMapControl (Window, 3, 24, 23, 480, 360, 4, "USERNOTE","RONOTE")
	Map = GemRB.GetControl (Window, 3)
	GemRB.SetVar ("x",1)
	GemRB.SetVarAssoc (Window, Map, "x", 1)

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
	Label = GemRB.GetControl (MapWindow, 4)
	Text = GemRB.QueryText (MapWindow, Label)
	GemRB.SetMapNote (PosX, PosY, 0, Text)
	return
	
def SetMapNote ():
	global PosX, PosY

	Label = GemRB.GetControl (MapWindow, 4)
	GemRB.SetEvent (MapWindow, Label, IE_GUI_EDIT_ON_CHANGE, "NoteChanged")
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	GemRB.SetControlStatus (MapWindow, Label, IE_GUI_CONTROL_FOCUSED)
	Map = GemRB.GetControl (MapWindow, 3)
	GemRB.SetVar ("x",1)
	GemRB.SetVarAssoc (MapWindow, Map, "x", 1)
	return

def AddMapNote ():
	Map = GemRB.GetControl (MapWindow, 3)
	GemRB.SetVar ("x",2)
	GemRB.SetVarAssoc (MapWindow, Map, "x", 2)
	return

###################################################
# End of file GUIMA.py
def OpenWorldMapWindow ():
	global WorldMapWindow, Travel

	Travel = 1 #allow travel or not, this should be set somehow
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

	#Button = GemRB.GetControl (Window, 4)
	#GemRB.SetControlSize (Window, Button, 0, 0)
	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel)
	Map = GemRB.GetControl (Window, 4)
	
	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")

	GemRB.UnhideGUI ()


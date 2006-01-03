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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg2/GUIMA.py,v 1.27 2006/01/03 17:16:04 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

MapWindow = None
NoteWindow = None
WorldMapWindow = None
WorldMapControl = None
OldPortraitWindow = None
OldOptionsWindow = None

###################################################
def OpenMapWindow ():
	global MapWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenMapWindow):
		GemRB.UnloadWindow (MapWindow)
		GemRB.UnloadWindow (OptionsWindow)
		GemRB.UnloadWindow (PortraitWindow)

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIMAP", 640, 480)
	MapWindow = Window = GemRB.LoadWindow (2)
	#this window type blocks the game normally, but map window doesn't
	GemRB.SetVar ("OtherWindow", MapWindow)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenMapWindow")
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)
	GemRB.SetWindowFrame (OptionsWindow)

	# World Map
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindowInside")

	# Hide or Show mapnotes
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetButtonFlags (Window, Button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	# Is this an option?
	GemRB.SetVar ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetVarAssoc (Window, Button, "ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

	Label = GemRB.GetControl (Window, 0x10000003)
	GemRB.SetText (Window, Label, "")

	# Map Control
	GemRB.CreateMapControl (Window, 2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	Map = GemRB.GetControl (Window, 2)
	GemRB.SetVarAssoc (Window, Map, "ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	GemRB.SetEvent (Window, Map, IE_GUI_MAP_ON_PRESS, "AddNoteWindow")
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)

def LeftDoublePressMap ():
	print "MoveToPoint"
	return

def CloseNoteWindow ():
	GemRB.SetVisible (NoteWindow, 0)
	GemRB.SetVisible (MapWindow, 1)
	return

def RemoveMapNote ():
	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")
	GemRB.SetMapnote (PosX, PosY, 0, "")
	CloseNoteWindow ()
	return

def SetMapNote ():
	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")
	Label = GemRB.GetControl (NoteWindow, 1)
	Text = GemRB.QueryText (NoteWindow, Label)
	Color = GemRB.GetVar ("Color")
	GemRB.SetMapnote (PosX, PosY, Color, Text)
	CloseNoteWindow ()
	return

def AddNoteWindow ():
	global NoteWindow

	Label = GemRB.GetControl (MapWindow, 0x10000003)
	Text = GemRB.QueryText (MapWindow, Label)
	NoteWindow = GemRB.LoadWindow (5)
	Label = GemRB.GetControl (NoteWindow, 1)
	GemRB.SetText (NoteWindow, Label, Text)
	GemRB.SetControlStatus (NoteWindow, Label, IE_GUI_CONTROL_FOCUSED)
	for i in range(8):
		Label = GemRB.GetControl (NoteWindow, 4+i)
		#the .chu is crappy, we have to reset the flags
		GemRB.SetButtonSprites (NoteWindow, Label, "FLAG1", i,0,1,2,0)
		GemRB.SetButtonFlags (NoteWindow, Label, IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		GemRB.SetVarAssoc (NoteWindow, Label, "Color", i)

	#set
	Label = GemRB.GetControl (NoteWindow, 0)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"SetMapNote")
	GemRB.SetButtonFlags (NoteWindow, Label, IE_GUI_BUTTON_DEFAULT, OP_OR)
	GemRB.SetText (NoteWindow, Label, 11973)

	#cancel
	Label = GemRB.GetControl (NoteWindow, 2)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"CloseNoteWindow")
	GemRB.SetText (NoteWindow, Label, 13727)

	#remove
	Label = GemRB.GetControl (NoteWindow, 3)
	GemRB.SetEvent (NoteWindow, Label, IE_GUI_BUTTON_ON_PRESS,"RemoveMapNote")
	GemRB.SetText (NoteWindow, Label, 13957)

	GemRB.SetVisible (MapWindow, 2)
	GemRB.SetVisible (NoteWindow, 1)
	return

def OpenWorldMapWindowInside ():
	OpenMapWindow() #closes mapwindow
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

def WorldMapWindowCommon(Travel):
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		CloseWorldMapWindow()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIWMAP", 640, 480)
	WorldMapWindow = Window = GemRB.LoadWindow (0)
	#saving the original portrait window
	GemRB.SetWindowFrame (Window)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel, "floattxt")
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

	#northwest
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapNW")

	#northeast
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapNE")

	#west
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapW")

	#center
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapC")

	#east
	Button = GemRB.GetControl (Window, 12)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapE")

	#southwest
	Button = GemRB.GetControl (Window, 13)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapSW")

	#southeast
	Button = GemRB.GetControl (Window, 14)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "MapSE")

	# Done
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseWorldMapWindow")
	GemRB.SetVisible (Window, 1)
	return

def MapN():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 0, -10)
	return

def MapNE():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 10, -10)
	return

def MapE():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 10, 0)
	return

def MapSE():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 10, 10)
	return

def MapS():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, 0, 10)
	return

def MapSW():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, -10, 10)
	return

def MapW():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, -10, 0)
	return

def MapNW():
	GemRB.AdjustScrolling (WorldMapWindow, WorldMapControl, -10, -10)
	return

def MapC():
	return

###################################################
# End of file GUIMA.py

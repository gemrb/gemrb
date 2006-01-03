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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd2/GUIMA.py,v 1.7 2006/01/03 19:48:22 avenger_teambg Exp $


# GUIMA.py - scripts to control map windows from GUIMA and GUIWMAP winpacks

###################################################

import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow

MapWindow = None
NoteWindow = None
WorldMapWindow = None

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
	GemRB.LoadWindowPack ("GUIMAP")
	MapWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", MapWindow)

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
	GemRB.UnhideGUI ()

def LeftDoublePressMap ():
	print "MoveToPoint"
	return

def CloseNoteWindow ():
	GemRB.SetVisible (NoteWindow, 0)
	GemRB.SetVisible (MapWindow, 1)
	return

def RemoveMapNote ():
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	GemRB.SetMapnote (PosX, PosY, 0, "")
	CloseNoteWindow ()
	return

def SetMapNote ():
	PosX = GemRB.GetVar("MapControlX")
	PosY = GemRB.GetVar("MapControlY")
	Label = GemRB.GetControl (NoteWindow, 1)
	Text = GemRB.QueryText (NoteWindow, Label)
	Color = GemRB.GetVar("Color")
	GemRB.SetMapnote (PosX, PosY, Color, Text)
	CloseNoteWindow ()
	return

def AddNoteWindow ():
	global NoteWindow

	Label = GemRB.GetControl (MapWindow, 0x10000003)
	Text = GemRB.QueryText (MapWindow, Label)
	NoteWindow = GemRB.LoadWindow (5)
	Label = GemRB.GetControl (NoteWindow, 1)
	GemRB.SetBufferLength (NoteWindow, Label, 100)
	GemRB.SetText (NoteWindow, Label, Text)
	GemRB.SetControlStatus (NoteWindow, Label, IE_GUI_CONTROL_FOCUSED)
	for i in range(8):
		Label = GemRB.GetControl (NoteWindow, 4+i)
		GemRB.SetButtonSprites (NoteWindow, Label, "FLAG1", i,0,1,2,0)
		GemRB.SetButtonFlags (NoteWindow, Label, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
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

def WorldMapWindowCommon (Travel):
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		CloseWorldMapWindow ()
		return

        GemRB.HideGUI ()
        GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIWMAP",800, 600)
	WorldMapWindow = Window = GemRB.LoadWindow (2)

	GemRB.CreateWorldMapControl (Window, 4, 0, 62, 640, 418, Travel, "infofont")
        WorldMapControl = GemRB.GetControl (Window, 4)
        GemRB.SetAnimation (Window, WorldMapControl, "WMDAG")
        GemRB.SetEvent (Window, WorldMapControl, IE_GUI_WORLDMAP_ON_PRESS, "MoveToNewArea")
        GemRB.SetEvent (Window, WorldMapControl, IE_GUI_MOUSE_ENTER_WORLDMAP, "ChangeTooltip")

	# Done
	Button = GemRB.GetControl (Window, 0)
	if Travel>=0:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")
	else:
		GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")
	GemRB.SetVisible (Window, 1)

###################################################
# End of file GUIMA.py

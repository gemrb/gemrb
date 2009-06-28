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
# $Id$


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
OptionsWindow = None
OldOptionsWindow = None
PortraitWindow = None
OldPortraitWindow = None

def RevealMap ():
	global MapWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (ShowMap):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MapWindow = None
		#this window type should block the game
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
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
	global MapWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (ShowMap):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

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
	MapWindow = Window = GemRB.LoadWindowObject (2)
	#this window type blocks the game normally, but map window doesn't
	GemRB.SetVar ("OtherWindow", MapWindow.ID)
	#saving the original portrait window
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 0, "ShowMap")
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)
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
	Window.CreateMapControl (2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	Map = Window.GetControl (2)
	GemRB.SetVar ("ShowMapNotes",IE_GUI_MAP_REVEAL_MAP)
	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_REVEAL_MAP)
	Map.SetEvent (IE_GUI_MAP_ON_PRESS, "RevealMap")
	Window.SetVisible (1)
	OptionsWindow.SetVisible (2)
	PortraitWindow.SetVisible (2)
	OptionsWindow.SetVisible (3)
	PortraitWindow.SetVisible (3)
	Window.SetVisible (3)
	Map.SetStatus(IE_GUI_CONTROL_FOCUSED)
	GemRB.GamePause (0,0)
	return

###################################################
def OpenMapWindow ():
	global MapWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenMapWindow):
		if MapWindow:
			MapWindow.Unload ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		if PortraitWindow:
			PortraitWindow.Unload ()

		MapWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		GemRB.SetVisible (0,1)
		GemRB.UnhideGUI ()
		GUICommonWindows.PortraitWindow = OldPortraitWindow
		OldPortraitWindow = None
		GUICommonWindows.OptionsWindow = OldOptionsWindow
		OldOptionsWindow = None
		SetSelectionChangeHandler (None)
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIMAP", 800, 600)
	MapWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", MapWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindowObject (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenMapWindow")
	OptionsWindow.SetFrame ()

	# World Map
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindowInside")

	# Hide or Show mapnotes
	Button = Window.GetControl (3)
	Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	# Is this an option?
	GemRB.SetVar ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

	Label = Window.GetControl (0x10000003)
	Label.SetText ("")

	# Map Control
	Window.CreateMapControl (2, 0, 0, 0, 0, 0x10000003, "FLAG1")
	Map = Window.GetControl (2)
	Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
	Map.SetEvent (IE_GUI_MAP_ON_RIGHT_PRESS, "AddNoteWindow")
	Map.SetEvent (IE_GUI_MAP_ON_DOUBLE_PRESS, "LeftDoublePressMap")
	OptionsWindow.SetVisible( 1)
	PortraitWindow.SetVisible( 1)
	Window.SetVisible( 1)

def LeftDoublePressMap ():
	OpenMapWindow()
	print "MoveToPoint"
	return

def CloseNoteWindow ():
	if NoteWindow:
		NoteWindow.Unload ()
	MapWindow.SetVisible (1)
	return

def RemoveMapNote ():
	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")
	GemRB.SetMapnote (PosX, PosY, 0, "")
	CloseNoteWindow ()
	return

def QueryText ():
	Data = ""
	row = 0
	while 1:
		GemRB.SetVar ("row", row)
		NoteLabel.SetVarAssoc ("row", row)
		line = NoteLabel.QueryText ()
		if len(line)<=0:
			break
		Data += line+"\n"
		row += 1
	return Data

def SetMapNote ():
	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")
	Label = NoteWindow.GetControl (1)
	Text = QueryText ()
	Color = GemRB.GetVar ("Color")
	GemRB.SetMapnote (PosX, PosY, Color, Text)
	CloseNoteWindow ()
	return

def SetFocusBack ():
	NoteLabel.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def AddNoteWindow ():
	global NoteWindow, NoteLabel

	Label = MapWindow.GetControl (0x10000003)
	Text = Label.QueryText ()
	NoteWindow = GemRB.LoadWindowObject (5)
	NoteLabel = NoteWindow.GetControl (1)
	NoteLabel = NoteLabel.ConvertEdit (1)
	NoteLabel.SetText (Text)
	NoteLabel.SetStatus (IE_GUI_CONTROL_FOCUSED)

	for i in range(8):
		Label = NoteWindow.GetControl (4+i)
		Label.SetSprites ("FLAG1", i,0,1,2,0)
		Label.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Label.SetVarAssoc ("Color", i)
		Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SetFocusBack")

	#convert to multiline, destroy unwanted resources

	#set
	Label = NoteWindow.GetControl (0)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS,"SetMapNote")
	Label.SetText (11973)
	Label.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	#cancel
	Label = NoteWindow.GetControl (2)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS,"CloseNoteWindow")
	Label.SetText (13727)
	Label.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	#remove
	Label = NoteWindow.GetControl (3)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS,"RemoveMapNote")
	Label.SetText (13957)

	MapWindow.SetVisible (2)
	NoteWindow.SetVisible (1)
	return

def OpenWorldMapWindowInside ():
	WorldMapWindowCommon (-1)
	return

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def MoveToNewArea ():
	global WorldMapWindow, WorldMapControl

	tmp = WorldMapControl.GetDestinationArea (1)
	CloseWorldMapWindow ()
	GemRB.CreateMovement (tmp["Destination"], tmp["Entrance"])
	return

def ChangeTooltip ():
	global WorldMapWindow, WorldMapControl
	global str

	tmp = WorldMapControl.GetDestinationArea ()
	if (tmp):
		str = "%s: %d"%(GemRB.GetString(23084),tmp["Distance"])
	else:
		str=""

	WorldMapControl.SetTooltip (str)
	return

def CloseWorldMapWindow ():
	global WorldMapWindow, WorldMapControl

	if WorldMapWindow:
		WorldMapWindow.Unload ()
	if PortraitWindow:
		PortraitWindow.Unload ()
	if OptionsWindow:
		OptionsWindow.Unload ()
	WorldMapWindow = None
	WorldMapControl = None
	GemRB.SetVar ("OtherWindow", -1)
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
	WorldMapWindow = Window = GemRB.LoadWindowObject (2)

	Window.CreateWorldMapControl (4, 0, 62, 640, 418, Travel, "infofont")
	WorldMapControl = Window.GetControl (4)
	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, "MoveToNewArea")
	WorldMapControl.SetEvent (IE_GUI_MOUSE_ENTER_WORLDMAP, "ChangeTooltip")

	# Done
	Button = Window.GetControl (0)
	if Travel>=0:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenWorldMapWindow")
	else:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenMapWindow")
	Window.SetVisible (1)

###################################################
# End of file GUIMA.py

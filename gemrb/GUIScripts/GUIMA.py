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
import GUICommonWindows
import GUIMACommon
from GUIDefines import *

MapWindow = None
NoteWindow = None
WorldMapControl = None

def RevealMap ():
	global MapWindow

	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")

	GemRB.RevealArea (PosX, PosY, 30, 1)
	GemRB.GamePause (0,0)
	
	MapWindow.Close()
	return

###################################################
# for farsight effect
###################################################
def ShowMap ():
	Window = OpenMapWindow()

	# World Map
	Button = Window.GetControl (1)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	Map = Window.GetControl (2)

	# Hide or Show mapnotes
	if HasMapNotes ():
		Button = Window.GetControl (3)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

		Label = Window.GetControl (0x10000003)
		Label.SetText ("")
	
		Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_REVEAL_MAP)

	Map.SetEvent (IE_GUI_MAP_ON_PRESS, RevealMap)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	GemRB.GamePause (0,0)
	return

###################################################

def InitMapWindow (Window):
	global MapWindow
	MapWindow = Window

	# World Map
	Button = Window.GetControl (1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenWorldMapWindow())

	# Hide or Show mapnotes
	if HasMapNotes ():
		Button = Window.GetControl (3)
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		# Is this an option?
		Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

		Label = Window.GetControl (0x10000003)
		Label.SetText ("")

	# Map Control
	if GameCheck.IsBG2() or GameCheck.IsIWD2():
		Map = Window.ReplaceSubview(2, IE_GUI_MAP, Label)
	else:
		Map = Window.ReplaceSubview(2, IE_GUI_MAP)

	Map.SetAction(lambda: Window.Close(), IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, 0, 2)

	if HasMapNotes ():
		Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
		Map.SetEvent (IE_GUI_MAP_ON_RIGHT_PRESS, AddNoteWindow)
		Map.SetStatus (IE_GUI_MAP_VIEW_NOTES)

	Map.Focus()

	return
	
def InitWorldMapWindow (Window):
	WorldMapWindowCommon (Window, -1)
	return

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMAP", GUICommonWindows.ToggleWindow, InitMapWindow)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMAP", GUICommonWindows.OpenWindowOnce, InitMapWindow)

if GameCheck.IsIWD2():
	OpenWorldMapWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIWMAP", GUICommonWindows.OpenWindowOnce, InitWorldMapWindow)
else:
	OpenWorldMapWindow = GUICommonWindows.CreateTopWinLoader(0, "GUIWMAP", GUICommonWindows.OpenWindowOnce, InitWorldMapWindow)

def HasMapNotes ():
	return GameCheck.IsBG2() or GameCheck.IsIWD2() or GameCheck.IsPST()

def SetMapNote (Text):
	PosX = GemRB.GetVar ("MapControlX")
	PosY = GemRB.GetVar ("MapControlY")
	Color = GemRB.GetVar ("Color")
	GemRB.SetMapnote (PosX, PosY, Color, Text)
	NoteWindow.Close()
	return

def AddNoteWindow ():
	global NoteWindow, NoteLabel, MapWindow

	Label = MapWindow.GetControl (0x10000003)
	Text = Label.QueryText ()
	if Text == "":
		Text = "Note"

	NoteWindow = GemRB.LoadWindow (5, "GUIMAP")
	NoteWindow.SetFlags(WF_ALPHA_CHANNEL, OP_OR)

	if GameCheck.IsIWD2():
		#convert to multiline, destroy unwanted resources
		NoteLabel = NoteWindow.ReplaceSubview(1, IE_GUI_TEXTAREA, "NORMAL")
		NoteLabel.SetFlags(IE_GUI_TEXTAREA_EDITABLE, OP_OR)

		# center relative to map
		mapframe = MapWindow.GetFrame()
		noteframe = NoteWindow.GetFrame()
		noteframe['x'] = mapframe['x'] + mapframe['w']/2 - noteframe['w']/2 - 60
		noteframe['y'] = mapframe['y'] + mapframe['h']/2 - noteframe['h']/2
		NoteWindow.SetFrame(noteframe)
	else:
		NoteLabel = NoteWindow.GetControl (1)
		NoteLabel.SetBackground (None)

	NoteLabel.SetText (Text)

	for i in range(8):
		Label = NoteWindow.GetControl (4+i)
		#the .chu is crappy, we have to reset the flags
		Label.SetSprites ("FLAG1", i,0,1,2,0)
		Label.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Label.SetVarAssoc ("Color", i)
		Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: NoteLabel.Focus())

	#set
	Label = NoteWindow.GetControl (0)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: SetMapNote(NoteLabel.QueryText()))
	Label.SetText (11973)
	Label.MakeDefault()

	#cancel
	Label = NoteWindow.GetControl (2)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: NoteWindow.Close())
	Label.SetText (13727)
	Label.MakeEscape()

	#remove
	Label = NoteWindow.GetControl (3)
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: SetMapNote(""))
	Label.SetText (13957)

	NoteWindow.ShowModal (MODAL_SHADOW_GRAY)
	NoteLabel.Focus()
	return
	
def OpenTravelWindow ():
	Window = OpenWorldMapWindow()
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

	Window.SetFlags(WF_ALPHA_CHANNEL, OP_NAND)

	if GameCheck.IsBG2():
		WorldMapControl = Window.ReplaceSubview (4, IE_GUI_WORLDMAP, Travel, "floattxt")
	elif GameCheck.IsBG1():
		WorldMapControl = Window.ReplaceSubview (4, IE_GUI_WORLDMAP, Travel, "toolfont", 1, True)
		WorldMapControl.SetTextColor (IE_GUI_WMAP_COLOR_BACKGROUND, {'r' : 0xa4, 'g' : 0x6a, 'b' : 0x4c})
	else:
		WorldMapControl = Window.ReplaceSubview (4, IE_GUI_WORLDMAP, Travel, "infofont")

	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, GUIMACommon.MoveToNewArea)
	WorldMapControl.SetAction(ChangeTooltip, IE_ACT_MOUSE_ENTER)
	# center on current area
	MapC()

	if not GameCheck.IsIWD2():
		#north
		Button = Window.GetControl (1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapN)
		Button.SetActionInterval (200)

		#south
		Button = Window.GetControl (2)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapS)
		Button.SetActionInterval (200)

	if GameCheck.IsBG2():
		#northwest
		Button = Window.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapNW)
		Button.SetActionInterval (200)

		#northeast
		Button = Window.GetControl (9)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapNE)
		Button.SetActionInterval (200)

		#west
		Button = Window.GetControl (10)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapW)
		Button.SetActionInterval (200)

		#center
		Button = Window.GetControl (11)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapC)
		Button.SetActionInterval (200)

		#east
		Button = Window.GetControl (12)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapE)
		Button.SetActionInterval (200)

		#southwest
		Button = Window.GetControl (13)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapSW)
		Button.SetActionInterval (200)

		#southeast
		Button = Window.GetControl (14)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapSE)
		Button.SetActionInterval (200)

	# Done
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: OpenMapWindow ())
	Button.SetHotKey('m')

	return

def MapN():
	WorldMapControl.Scroll (0, -10)
	return

def MapNE():
	WorldMapControl.Scroll (10, -10)
	return

def MapE():
	WorldMapControl.Scroll (10, 0)
	return

def MapSE():
	WorldMapControl.Scroll (10, 10)
	return

def MapS():
	WorldMapControl.Scroll (0, 10)
	return

def MapSW():
	WorldMapControl.Scroll (-10, 10)
	return

def MapW():
	WorldMapControl.Scroll (-10, 0)
	return

def MapNW():
	WorldMapControl.Scroll (-10, -10)
	return

def MapC():
	WorldMapControl.Scroll (0, 0, False)
	return

###################################################
# End of file GUIMA.py

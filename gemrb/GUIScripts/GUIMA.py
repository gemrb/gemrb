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
from GUIDefines import *

MapWindow = None
NoteWindow = None
WorldMapWindow = None
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

	Map = Window.GetControl (1000)

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
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: WorldMapWindowCommon (-1))

	# Hide or Show mapnotes
	if HasMapNotes ():
		Button = Window.GetControl (3)
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		# Is this an option?
		Button.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)

		Label = Window.GetControl (0x10000003)
		Label.SetText ("")

	# Map Control
	placeholder = Window.GetControl (2)
	if GameCheck.IsBG2() or GameCheck.IsIWD2():
		Map = Window.CreateMapControl (1000, 0, 0, 0, 0, Label, "FLAG1")
	else:
		Map = Window.CreateMapControl (1000, 0, 0, 0, 0)

	Map.SetFrame(placeholder.GetFrame())
	Window.DeleteControl (placeholder)

	if HasMapNotes ():
		Map.SetVarAssoc ("ShowMapNotes", IE_GUI_MAP_VIEW_NOTES)
		Map.SetEvent (IE_GUI_MAP_ON_RIGHT_PRESS, AddNoteWindow)
		Map.SetStatus (IE_GUI_MAP_VIEW_NOTES)

	Map.Focus()

	return

ToggleMapWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMAP", GUICommonWindows.ToggleWindow, InitMapWindow)
OpenMapWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIMAP", GUICommonWindows.OpenWindowOnce, InitMapWindow)

def HasMapNotes ():
	return GameCheck.IsBG2() or GameCheck.IsIWD2() or GameCheck.IsPST()

def QueryText ():
	if not GameCheck.IsIWD2():
		return  NoteLabel.QueryText ()

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
	NoteWindow = GemRB.LoadWindow (5)
	NoteWindow.SetFlags(WF_ALPHA_CHANNEL, OP_OR)
	NoteLabel = NoteWindow.GetControl (1)
	if GameCheck.IsIWD2():
		#convert to multiline, destroy unwanted resources
		#0 is the default Scrollbar ID
		NoteTA = MapWindow.CreateTextArea(100, 0, 0, 0, 0, "NORMAL", IE_FONT_ALIGN_CENTER) # ID/position/size dont matter. we will substitute later
		NoteLabel = NoteTA.SubstituteForControl(NoteLabel)
	else:
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
	Label.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: SetMapNote(QueryText()))
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

def OpenWorldMapWindow ():
	WorldMapWindowCommon (GemRB.GetVar ("Travel"))
	return

def MoveToNewArea ():
	global WorldMapWindow, WorldMapControl

	tmp = WorldMapControl.GetDestinationArea (1)
	hours = tmp["Distance"]
	OpenWorldMapWindow ()

	if tmp["Destination"].lower() == GemRB.GetGameString(STR_AREANAME).lower():
		return
	elif hours == -1:
		print "Invalid target", tmp
		return

	GemRB.CreateMovement (tmp["Destination"], tmp["Entrance"], tmp["Direction"])

	if hours == 0:
		return

	# distance is stored in hours, but the action needs seconds
	GemRB.ExecuteString ("AdvanceTime(%d)"%(hours*300))

	# ~The journey took <DURATION>.~ but pst has it without the token
	# HOWEVER: pst has its own GUIMA + uses only neighbour travel (no MoveToNewArea)??
	if GameCheck.IsPST():
		# GemRB.DisplayString can only deal with resrefs, so cheat until noticed
		if hours > 1:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + GemRB.GetString (19313))
		else:
			GemRB.Log (LOG_MESSAGE, "Actor", GemRB.GetString (19261) + str(hours) + GemRB.GetString (19312))
	else:
		time = ""
		GemRB.SetToken ("HOUR", str(hours))
		if hours > 1:
			time =  GemRB.GetString (10700)
		else:
			time =  GemRB.GetString (10701)
		GemRB.SetToken ("DURATION", time)
		GemRB.DisplayString (10689, 0xffffff)
	return

def WorldMapWindowCommon (Travel):
	global WorldMapWindow, WorldMapControl

	WorldMapWindow = Window = GemRB.LoadWindow (0, "GUIWMAP")

	if GameCheck.IsBG2():
		WorldMapControl = Window.CreateWorldMapControl (4, 0, 105, 640, 375, Travel, "floattxt")
	elif GameCheck.IsBG1():
		WorldMapControl = Window.CreateWorldMapControl (4, 0, 105, 640, 375, Travel, "toolfont", 1)
		WorldMapControl.SetTextColor (IE_GUI_WMAP_COLOR_BACKGROUND, 0xa4, 0x6a, 0x4c, 0x00)
	else:
		WorldMapControl = Window.CreateWorldMapControl (4, 0, 105, 640, 375, Travel, "infofont")

	WorldMapControl.SetAnimation ("WMDAG")
	WorldMapControl.SetEvent (IE_GUI_WORLDMAP_ON_PRESS, MoveToNewArea)
	# center on current area
	MapC()

	if not GameCheck.IsIWD2():
		#north
		Button = Window.GetControl (1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapN)

		#south
		Button = Window.GetControl (2)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapS)

	if GameCheck.IsBG2():
		#northwest
		Button = Window.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapNW)

		#northeast
		Button = Window.GetControl (9)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapNE)

		#west
		Button = Window.GetControl (10)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapW)

		#center
		Button = Window.GetControl (11)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapC)

		#east
		Button = Window.GetControl (12)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapE)

		#southwest
		Button = Window.GetControl (13)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapSW)

		#southeast
		Button = Window.GetControl (14)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MapSE)

	# Done
	Button = Window.GetControl (0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: WorldMapWindow.Close())
	Button.MakeEscape()

	return

def MapN():
	WorldMapControl.AdjustScrolling (0, -10)
	return

def MapNE():
	WorldMapControl.AdjustScrolling (10, -10)
	return

def MapE():
	WorldMapControl.AdjustScrolling (10, 0)
	return

def MapSE():
	WorldMapControl.AdjustScrolling (10, 10)
	return

def MapS():
	WorldMapControl.AdjustScrolling (0, 10)
	return

def MapSW():
	WorldMapControl.AdjustScrolling (-10, 10)
	return

def MapW():
	WorldMapControl.AdjustScrolling (-10, 0)
	return

def MapNW():
	WorldMapControl.AdjustScrolling (-10, -10)
	return

def MapC():
	WorldMapControl.AdjustScrolling (0, 0)
	return

###################################################
# End of file GUIMA.py

# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
#################### Interchapter text screen functions #################

import GemRB
from GUIDefines import *
import GUICommon

TextScreen = None
TextArea = None
Row = 1
Position = 1
Chapter = 0
TableName = None
BGTICKS = 300 # TODO: verify for suitability
Ticks = IWDTICKS = 200 # TODO: verify for suitability in iwd1

def StartTextScreen ():
	global TextScreen, TextArea, Chapter, TableName, Row, Ticks

	if GUICommon.GameIsIWD2():
		GemRB.LoadWindowPack ("GUICHAP", 800, 600)
	else:
		GemRB.LoadWindowPack ("GUICHAP", 640, 480)
		if GUICommon.GameIsBG1() or GUICommon.GameIsBG2():
			Ticks = BGTICKS

	LoadPic = TableName = GemRB.GetGameString (STR_LOADMOS)
	#if there is no preset loadpic, try to determine it from the chapter
	#fixme: we always assume there isn't for non-bg2
	if GUICommon.GameIsBG2():
		if TableName == "":
			Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
			TableName = "CHPTXT"+str(Chapter)
		ID = 62
	else:
		ID = (GemRB.GetGameVar("CHAPTER") + 1) & 0x7fffffff
		Chapter = ID + 1

	if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
		#fixme: this is also a guess; are there more, one per chapter?
		if LoadPic == "":
			GemRB.LoadMusicPL ("chap0.mus")
		else:
			GemRB.LoadMusicPL ("chap1.mus")
		TableName = "chapters"
	elif GUICommon.GameIsBG1() and TableName[:6] == "chptxt":
		GemRB.LoadMusicPL ("chapter.mus")
	#else: TODO: what about bg2?

	TextScreen = GemRB.LoadWindow (ID)
	TextScreen.SetFrame ()

	TextArea = TextScreen.GetControl (2)
	TextArea.SetFlags (IE_GUI_TEXTAREA_SMOOTHSCROLL)
	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")

	#caption
	Table = GemRB.LoadTable (TableName)
	if GUICommon.GameIsBG1():
		#these suckers couldn't use a fix row
		Row = Table.GetRowIndex("DEFAULT")
		Value = Table.GetValue (Row, 0)
	elif GUICommon.GameIsBG2():
		LoadPic = Table.GetValue (-1, -1)
		if LoadPic != "":
			TextScreen.SetPicture (LoadPic)
		# TODO: verify
		Value = 0
	else:
		Value = Table.GetValue (Chapter, 0)
	if Value:
		Label=TextScreen.GetControl (0x10000000)
		Label.SetText (Value)

	#done
	Button=TextScreen.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "EndTextScreen")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT|IE_GUI_BUTTON_CANCEL,OP_OR)

	#replay
	Button=TextScreen.GetControl (3)
	Button.SetText (16510)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ReplayTextScreen")

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE) #removing the gamecontrol screen
	TextScreen.SetVisible (WINDOW_VISIBLE)

	TextArea.Rewind (Ticks)
	GemRB.DisplayString (17556, 0xff0000) #Paused for chapter text
	GemRB.GamePause (1, 1)
	return

def FeedScroll ():
	global TextArea, Position

	Table = GemRB.LoadTable (TableName)
	if GUICommon.GameIsBG2():
		#this is a rather primitive selection but works for the games
		Value = Table.GetValue (0, 1)
		if Value == "REPUTATION":
			line = 2
		else:
			line = 1
		Value = Table.GetValue (line, 1)
	elif GUICommon.GameIsBG1():
		Value = Table.GetValue (Row, Position)
		if Value == 'NONE':
			Position = 1
		else:
			Position = Position + 1
	else:
		Value = Table.GetValue (Chapter, 1)

	TextArea.Append (Value, -1, 6)
	return

def EndTextScreen ():
	global TextScreen

	TextScreen.SetVisible (WINDOW_INVISIBLE)
	if TextScreen:
		TextScreen.Unload ()
		GemRB.PlaySound(None, 0, 0, 4)
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) # enable the gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause (0, 1)
	return

def ReplayTextScreen ():
	global TextArea, Position

	Position = 1
	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")
	TextArea.Rewind (Ticks)
	return

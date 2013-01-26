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
Chapter = 0
TableName = None

def FindTextRow (Table):
	global Row

	#this is still not the full implementation, but the original engine never used this stuff
	Row = Table.GetRowIndex("DEFAULT")
	if Table.GetValue (Row, 1)== -1:
		if GemRB.GameGetReputation() >= 100:
			Row = Table.GetRowIndex("GOOD_REPUTATION")
		else:
			Row = Table.GetRowIndex("BAD_REPUTATION")
	return

def StartTextScreen ():
	global TextScreen, TextArea, Chapter, TableName, Row

	GemRB.GamePause (1, 3)

	LoadPic = TableName = GemRB.GetGameString (STR_TEXTSCREEN)
	if TableName[:6] == "drmtxt":
		GemRB.DisplayString (17558, 0xff0000) #Paused for rest
	else:
		GemRB.DisplayString (17556, 0xff0000) #Paused for chapter text

	if GUICommon.GameIsIWD2():
		GemRB.LoadWindowPack ("GUICHAP", 800, 600)
	else:
		GemRB.LoadWindowPack ("GUICHAP", 640, 480)

	#if there is no preset loadpic, try to determine it from the chapter
	#fixme: we always assume there isn't for non-bg2
	if GUICommon.GameIsBG2():
		if TableName == "":
			#apparently, BG2 doesn't always use IncrementChapter to trigger a TextScreen
			#Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
			#TableName = "CHPTXT"+str(Chapter)
			EndTextScreen ()
			return
		ID = 62
	elif TableName[:6] == "drmtxt":
		ID = 50 + (GemRB.GetGameVar("DREAM") & 0x7fffffff)
	elif TableName == "islon":
		ID = 58
	elif TableName == "isloff":
		ID = 59
	elif TableName == "toscst":
		ID = 60
	elif TableName == "toscend":
		ID = 61
	else:
		ID = GemRB.GetGameVar("CHAPTER") & 0x7fffffff
		Chapter = ID + 1

	#iwd2/bg2 has no separate music
	if GUICommon.GameIsIWD1():
		if LoadPic == "":
			GemRB.LoadMusicPL ("chap0.mus")
		else:
			GemRB.LoadMusicPL ("chap1.mus")
		TableName = "chapters"
	elif GUICommon.GameIsIWD2():
		GemRB.HardEndPL ()
		TableName = "chapters"
	elif GUICommon.GameIsBG1() and TableName[:6] == "chptxt":
		GemRB.LoadMusicPL ("chapter.mus")
	else:
		GemRB.HardEndPL ()

	TextScreen = GemRB.LoadWindow (ID)
	TextScreen.SetFrame ()

	TextArea = TextScreen.GetControl (2)
	TextArea.SetFlags (IE_GUI_TEXTAREA_SMOOTHSCROLL)

	#caption
	Table = GemRB.LoadTable (TableName)
	if GUICommon.GameIsBG1():
		#these suckers couldn't use a fix row
		FindTextRow (Table)
	elif GUICommon.GameIsBG2():
		LoadPic = Table.GetValue (-1, -1)
		if LoadPic != "":
			TextScreen.SetPicture (LoadPic)
		FindTextRow (Table)
	else:
		Row = Chapter

	Value = Table.GetValue (Row, 0)
	#don't display the fake -1 string (No caption in toscst.2da)
	if Value!="NONE" and Value>0 and TextScreen.HasControl(0x10000000):
		Label=TextScreen.GetControl (0x10000000)
		Label.SetText (Value)

	#done
	Button=TextScreen.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, EndTextScreen)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT|IE_GUI_BUTTON_CANCEL,OP_OR)

	#replay
	Button=TextScreen.GetControl (3)
	Button.SetText (16510)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReplayTextScreen)

	#if this was opened from somewhere other than game control close that window
	GUICommon.CloseOtherWindow(None)
	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE) #removing the gamecontrol screen
	TextScreen.SetVisible (WINDOW_VISIBLE)

	ReplayTextScreen()
	return

def EndTextScreen ():
	global TextScreen

	if TextScreen:
		TextScreen.SetVisible (WINDOW_INVISIBLE)
		TextScreen.Unload ()
		GemRB.PlaySound(None, 0, 0, 4)

	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE) # enable the gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause (0, 3)
	return

def ReplayTextScreen ():
	global TextArea

	TextArea.Rewind ()

	Table = GemRB.LoadTable (TableName)
	Count = Table.GetColumnCount (Row)

	#hack for messy chapters.2da in IWD
	#note: iwd doesn't use TextScreen, only IncrementChapter
	if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
		Count = 2

	for i in range(1, Count):
		TextArea.Append ("\n")
		# flag value of 14 = IE_STR_SOUND|IE_STR_SPEECH/GEM_SND_SPEECH|GEM_SND_QUEUE
		TextArea.Append (Table.GetValue (Row, i), -1, 14)

	return

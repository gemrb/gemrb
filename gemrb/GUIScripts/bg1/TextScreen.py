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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/bg1/TextScreen.py,v 1.7 2005/11/26 10:49:36 avenger_teambg Exp $

# TextScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

TextScreen = None
TextArea = None
Position = 1

def StartTextScreen ():
	global TextScreen, TextArea

	GemRB.LoadWindowPack ("GUICHAP", 640, 480)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	#if there is no preset loadpic, try to determine it from the chapter
	if LoadPic[:6] == "CHPTXT":
		Chapter = GemRB.GetVar("CHAPTER")
		#set ID according to the Chapter?
		ID = Chapter
		GemRB.LoadMusicPL("chapter.mus")
	else:
		ID = 62
	TextScreen = GemRB.LoadWindow (ID)
	GemRB.SetWindowFrame (TextScreen)
	TextArea = GemRB.GetControl (TextScreen, 2)
	GemRB.SetTextAreaFlags (TextScreen, TextArea, IE_GUI_TEXTAREA_SMOOTHSCROLL)
	GemRB.SetEvent (TextScreen, TextArea, IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")

	#caption
	TableName = GemRB.GetGameString (STR_LOADMOS)
	Table = GemRB.LoadTable(TableName)
	Value = GemRB.GetTableValue (Table, 2, 0)
	GemRB.UnloadTable (Table)
	Label=GemRB.GetControl (TextScreen, 0x10000000)
	GemRB.SetText (TextScreen, Label, Value)

	#done
	Button=GemRB.GetControl (TextScreen, 0)
	GemRB.SetText (TextScreen, Button, 11973)
	GemRB.SetEvent (TextScreen, Button, IE_GUI_BUTTON_ON_PRESS, "EndTextScreen")

	#replay
	Button=GemRB.GetControl (TextScreen, 3)
	GemRB.SetText (TextScreen, Button, 16510)
	GemRB.SetEvent (TextScreen, Button, IE_GUI_BUTTON_ON_PRESS, "ReplayTextScreen")

	GemRB.HideGUI ()
	GemRB.SetVisible (0, 0) #removing the gamecontrol screen
	GemRB.SetVisible (TextScreen, 1)
	GemRB.RewindTA(TextScreen, TextArea, 300)
	GemRB.GamePause(1, 1)


def FeedScroll ():
	global TextScreen, TextArea, Position

	TableName = GemRB.GetGameString (STR_LOADMOS)
	Table = GemRB.LoadTable(TableName)
	Value = GemRB.GetTableValue (Table, 2, Position)
	GemRB.UnloadTable (Table)
	if Value == 'NONE':
		Position = 1
	else:
		GemRB.TextAreaAppend (TextScreen, TextArea, Value, -1)
		Position = Position + 1


def ReplayTextScreen ():
	global TextScreen, TextArea, Position

	Position = 1
	GemRB.SetEvent (TextScreen, TextArea, IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")
	GemRB.RewindTA(TextScreen, TextArea, 300)


def EndTextScreen ():
	global TextScreen

	GemRB.SetVisible (TextScreen, 0)
	GemRB.UnloadWindow (TextScreen)
	GemRB.SetVisible (0, 1) #enabling gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause(0, 1)


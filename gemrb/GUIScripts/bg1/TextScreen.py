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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$

# TextScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

TextScreen = None
TextArea = None
Position = 1
Row = 1

def StartTextScreen ():
	global TextScreen, TextArea, Row

	GemRB.LoadWindowPack ("GUICHAP", 640, 480)
	TableName = GemRB.GetGameString (STR_LOADMOS)
	#if there is no preset loadpic, try to determine it from the chapter	
	if TableName[:6] == "chptxt":
		GemRB.LoadMusicPL("chapter.mus")

	print "TABLENAME:",TableName
	Chapter = GemRB.GetGameVar("CHAPTER")
	print "Chapter:",Chapter
	#set ID according to the Chapter?
	ID = (Chapter+1) & 0x7fffffff
	print "ID: ", ID
	TextScreen = GemRB.LoadWindowObject (ID)
	TextScreen.SetFrame ()
	TextArea = TextScreen.GetControl (2)
	TextArea.SetFlags (IE_GUI_TEXTAREA_SMOOTHSCROLL)
	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")

	#caption	
	Table = GemRB.LoadTableObject (TableName)

	#these suckers couldn't use a fix row
	Row = Table.GetRowIndex("DEFAULT")
	Value = Table.GetValue (Row, 0)
	Label=TextScreen.GetControl (0x10000000)
	Label.SetText (Value)

	#done
	Button=TextScreen.GetControl (0)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "EndTextScreen")
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT,OP_OR)

	#replay
	Button=TextScreen.GetControl (3)
	Button.SetText (16510)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "ReplayTextScreen")

	GemRB.HideGUI ()
	GemRB.SetVisible (0, 0) #removing the gamecontrol screen
	TextScreen.SetVisible (1)
	TextArea.Rewind (300)
	GemRB.DisplayString (17556, 0xff0000)
	GemRB.GamePause (1, 1)
	return

def FeedScroll ():
	global TextScreen, TextArea, Position

	TableName = GemRB.GetGameString (STR_LOADMOS)
	Table = GemRB.LoadTableObject (TableName)
	Value = Table.GetValue (Row, Position)
	if Value == 'NONE':
		Position = 1
	else:
		TextArea.Append (Value, -1, 6)
		Position = Position + 1
	return

def ReplayTextScreen ():
	global TextScreen, TextArea, Position

	Position = 1
	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")
	TextArea.Rewind (300)
	return

def EndTextScreen ():
	global TextScreen

	TextScreen.SetVisible (0)
	if TextScreen:
		TextScreen.Unload ()
	GemRB.SetVisible (0, 1) #enabling gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause (0, 1)
	return

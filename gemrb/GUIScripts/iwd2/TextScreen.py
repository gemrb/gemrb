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
# $Id$

# TextScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

TextScreen = None
TextArea = None
Chapter = 0

def StartTextScreen ():
	global TextScreen, TextArea, Chapter

	GemRB.LoadWindowPack ("GUICHAP", 800, 600)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	#if there is no preset loadpic, try to determine it from the chapter
	ID = GemRB.GetGameVar("CHAPTER")
	#set ID according to the Chapter?
	Chapter = ID + 1

	TextScreen = GemRB.LoadWindow (Chapter)
	GemRB.SetWindowFrame (TextScreen)

	TextArea = GemRB.GetControl (TextScreen, 2)
	GemRB.SetTextAreaFlags (TextScreen, TextArea, IE_GUI_TEXTAREA_SMOOTHSCROLL)
	GemRB.SetEvent (TextScreen, TextArea, IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")

	#caption
	Table = GemRB.LoadTable ("chapters")
	Value = GemRB.GetTableValue (Table, Chapter+1, 0)
	GemRB.UnloadTable (Table)
	Label=GemRB.GetControl (TextScreen, 0x10000000)
	GemRB.SetText (TextScreen, Label, Value)

	#done
	Button=GemRB.GetControl (TextScreen, 0)
	GemRB.SetText (TextScreen, Button, 11973)
	GemRB.SetEvent (TextScreen, Button, IE_GUI_BUTTON_ON_PRESS, "EndTextScreen")
	GemRB.SetButtonFlags (TextScreen, Button, IE_GUI_BUTTON_DEFAULT,OP_OR)

	#replay
	Button=GemRB.GetControl (TextScreen, 3)
	GemRB.SetText (TextScreen, Button, 16510)
	GemRB.SetEvent (TextScreen, Button, IE_GUI_BUTTON_ON_PRESS, "ReplayTextScreen")

	GemRB.HideGUI ()
	GemRB.SetVisible (0, 0) #removing the gamecontrol screen
	GemRB.SetVisible (TextScreen, 1)
	GemRB.RewindTA (TextScreen, TextArea, 200)
	GemRB.DisplayString (17556, 0xff0000)
	GemRB.GamePause(1, 1)
	return

def FeedScroll ():
	global TextScreen, TextArea

	Table = GemRB.LoadTable ("chapters")
	Value = GemRB.GetTableValue (Table, Chapter+1, 1)
	GemRB.UnloadTable (Table)
	GemRB.TextAreaAppend (TextScreen, TextArea, Value, -1, 6)
	return

def ReplayTextScreen ():
	global TextScreen, TextArea

	GemRB.SetEvent (TextScreen, TextArea, IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")
	GemRB.RewindTA (TextScreen, TextArea, 200)
	return

def EndTextScreen ():
	global TextScreen

	GemRB.SetVisible (TextScreen, 0)
	GemRB.UnloadWindow (TextScreen)
	GemRB.SetVisible (0, 1) #enabling gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause(0, 1)
	return

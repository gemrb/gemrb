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

# TextScreen.py - display Loading screen

###################################################

import GemRB
from GUIDefines import *

TextScreen = None
TextArea = None
Chapter = 0
Feed = 0

def StartTextScreen ():
	global TextScreen, TextArea, Chapter

	GemRB.LoadWindowPack ("GUICHAP", 640, 480)
	LoadPic = GemRB.GetGameString (STR_LOADMOS)
	#if there is no preset loadpic, try to determine it from the chapter
	#fixme: we always assume there isn't
	ID = (GemRB.GetGameVar("CHAPTER") + 1) & 0x7fffffff
	#set ID according to the Chapter?
	Chapter = ID + 1

	#this is also a guess
	if LoadPic == "":
		GemRB.LoadMusicPL ("chap0.mus")
	else:
		GemRB.LoadMusicPL ("chap1.mus")

	TextScreen = GemRB.LoadWindowObject (ID)
	TextScreen.SetFrame ()

	TextArea = TextScreen.GetControl (2)
	TextArea.SetFlags (IE_GUI_TEXTAREA_SMOOTHSCROLL)
	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")

	#fixme: this works only for chapter text, if there is other textscreen
	#then we should check LoadPic
	#caption
	Table = GemRB.LoadTableObject ("chapters")
	Value = Table.GetValue (Chapter, 0)
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
	GemRB.SetVisible (0, WINDOW_INVISIBLE) #removing the gamecontrol screen
	TextScreen.SetVisible (WINDOW_VISIBLE)
	TextArea.Rewind (200)
	GemRB.DisplayString (17556, 0xff0000)
	GemRB.GamePause (1, 1)
	return

def FeedScroll ():
	global TextScreen, TextArea, Feed

	if Feed:
		Feed = 0
		return

	Feed = 1
	Table = GemRB.LoadTableObject ("chapters")
	Value = Table.GetValue (Chapter, 1)
	TextArea.Append (Value, -1, 6)
	return

def ReplayTextScreen ():
	global TextScreen, TextArea, Feed

	TextArea.SetEvent (IE_GUI_TEXTAREA_OUT_OF_TEXT, "FeedScroll")
	Feed = 0
	TextArea.Rewind (200)
	return

def EndTextScreen ():
	global TextScreen

	TextScreen.SetVisible (WINDOW_INVISIBLE)
	if TextScreen:
		TextScreen.Unload ()
	GemRB.SetVisible (0, WINDOW_VISIBLE) #enabling gamecontrol screen
	GemRB.UnhideGUI ()
	GemRB.GamePause (0, 1)
	return

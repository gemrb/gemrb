# GemRB - Infinity Engine Emulator
# Copyright (C) 2003 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#instead of credits, you can listen the songs of the game :)
import GemRB
from GUIDefines import *

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	MovieWindow = GemRB.LoadWindow(2)
	MovieWindow.SetFrame ()
	TextAreaControl = MovieWindow.GetControl(0)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)
	MoviesTable = GemRB.LoadTable("MUSIC")
	for i in range(0, MoviesTable.GetRowCount() ):
			s = MoviesTable.GetRowName(i)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, PlayPress)
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	MovieWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	t = MoviesTable.GetValue(s, 0)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie("CREDITS")
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Unload()
	GemRB.SetNextScript("Start")
	return

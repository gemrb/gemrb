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
import GameCheck
import GUICommon

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 640, 480)
	MovieWindow = GemRB.LoadWindow(0)
	MovieWindow.SetFrame ()
	TextAreaControl = MovieWindow.GetControl(0)
	TextAreaControl.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)
	MoviesTable = GemRB.LoadTable("SONGLIST")
	for i in range(0, MoviesTable.GetRowCount() ):
			s = MoviesTable.GetValue(i, 0)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, PlayPress)
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DonePress)
	MovieWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	t = MoviesTable.GetValue(s, 1)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie("endcrdit", 1)
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Unload()
	if GameCheck.HasTOB():
		GemRB.SetNextScript ("Start2")
	else:
		GemRB.SetNextScript ("Start")
	return

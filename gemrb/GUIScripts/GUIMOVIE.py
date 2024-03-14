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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIMOVIE.py - Play Movies window

###################################################

import GemRB
import GameCheck
from GUIDefines import *

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	MovieWindow = GemRB.LoadWindow (0, "GUIMOVIE")
	TextAreaControl = MovieWindow.GetControl (0)
	PlayButton = MovieWindow.GetControl (2)
	CreditsButton = MovieWindow.GetControl (3)
	DoneButton = MovieWindow.GetControl (4)
	MoviesTable = GemRB.LoadTable ("MOVIDESC")
	opts = [MoviesTable.GetValue (i, 0) for i in range(MoviesTable.GetRowCount ()) if GemRB.GetVar(MoviesTable.GetRowName (i)) == 1]
	TextAreaControl.SetOptions (opts, "MovieIndex", 0)
	PlayButton.SetText (17318)
	CreditsButton.SetText (15591)
	DoneButton.SetText (11973)
	PlayButton.OnPress (PlayPress)
	CreditsButton.OnPress (CreditsPress)
	DoneButton.OnPress (MovieWindow.Close)
	DoneButton.MakeEscape()
	MovieWindow.Focus()
	return

def PlayPress():
	s = GemRB.GetVar ("MovieIndex")
	for i in range (MoviesTable.GetRowCount ()):
		t = MoviesTable.GetRowName (i)
		if GemRB.GetVar (t)==1:
			if s==0:
				s = MoviesTable.GetRowName (i)
				GemRB.PlayMovie (s, 1)
				return
			s = s - 1
	return

def CreditsPress():
	# arbitrary choice between custom jukebox and actual credits
	if GameCheck.IsBG1() or GameCheck.IsBG2():
		if MovieWindow:
			MovieWindow.Close ()
		GemRB.SetNextScript ("GUISONGS")
	else:
		GemRB.PlayMovie ("CREDITS",1)

	return

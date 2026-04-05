# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later


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
	if GameCheck.IsBG1() or GameCheck.IsBG2OrEE ():
		if MovieWindow:
			MovieWindow.Close ()
		GemRB.SetNextScript ("GUISONGS")
	else:
		GemRB.PlayMovie ("CREDITS",1)

	return

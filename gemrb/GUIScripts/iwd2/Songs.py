# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#instead of credits, you can listen the songs of the game :)
import GemRB
from GUIDefines import *

MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	MovieWindow = GemRB.LoadWindow(2, "GUIMOVIE")
	TextAreaControl = MovieWindow.GetControl(0)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)
	MoviesTable = GemRB.LoadTable("MUSIC")
	TextAreaControl.SetOptions([MoviesTable.GetRowName(i) for i in range(0, MoviesTable.GetRowCount() )], "MovieIndex", 0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	DoneButton.MakeEscape()

	PlayButton.OnPress (PlayPress)
	CreditsButton.OnPress (CreditsPress)
	DoneButton.OnPress (MovieWindow.Close)
	MovieWindow.Focus()
	return
	
def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	t = MoviesTable.GetValue(s, 0)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie("CREDITS", 1)
	return

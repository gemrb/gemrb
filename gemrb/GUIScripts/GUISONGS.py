# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#instead of credits, you can listen the songs of the game :)
import GemRB
import GameCheck

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0
MoviesTableName = ""
CreditsRef = ""
ColOffset = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable, MoviesTableName, ColOffset, CreditsRef

	MovieWindow = GemRB.LoadWindow(0, "GUIMOVIE")
	TextAreaControl = MovieWindow.GetControl(0)
	PlayButton = MovieWindow.GetControl(2)
	CreditsButton = MovieWindow.GetControl(3)
	DoneButton = MovieWindow.GetControl(4)

	if GameCheck.IsBG1():
		MoviesTableName = "MUSIC"
		ColOffset = 1
		CreditsRef = "credits"
	elif GameCheck.IsBG2OrEE ():
		MoviesTableName = "SONGLIST"
		ColOffset = 0
		CreditsRef = "endcrdit"
	MoviesTable = GemRB.LoadTable (MoviesTableName)
	TextAreaControl.SetOptions ([MoviesTable.GetValue (i, 0) for i in range(ColOffset, MoviesTable.GetRowCount())], "MovieIndex", 0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.OnPress (PlayPress)
	CreditsButton.OnPress (CreditsPress)
	DoneButton.OnPress (DonePress)
	MovieWindow.Focus()
	return
	
def PlayPress():
	s = GemRB.GetVar ("MovieIndex") + ColOffset
	t = MoviesTable.GetValue (s, 1-ColOffset)
	GemRB.LoadMusicPL(t,1)
	return

def CreditsPress():
	GemRB.PlayMovie (CreditsRef, 1)
	return

def DonePress():
	if MovieWindow:
		MovieWindow.Close ()
	if GameCheck.HasTOB():
		GemRB.SetNextScript ("Start2")
	else:
		GemRB.SetNextScript ("Start")
	return

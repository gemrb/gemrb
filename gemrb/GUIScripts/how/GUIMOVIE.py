# -*-python-*-
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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/how/GUIMOVIE.py,v 1.4 2004/10/08 20:44:22 edheldil Exp $


# GUIMOVIE.py - Play Movies window

###################################################

import GemRB

MovieWindow = 0
TextAreaControl = 0
MoviesTable = 0

def OnLoad():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE")
	MovieWindow = GemRB.LoadWindow(0)
	TextAreaControl = GemRB.GetControl(MovieWindow, 0)
	GemRB.SetTextAreaSelectable(MovieWindow, TextAreaControl,1)
	PlayButton = GemRB.GetControl(MovieWindow, 2)
	CreditsButton = GemRB.GetControl(MovieWindow, 3)
	DoneButton = GemRB.GetControl(MovieWindow, 4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			s = GemRB.GetTableValue(MoviesTable, i, 0)
			GemRB.TextAreaAppend(MovieWindow, TextAreaControl, s,-1)
	GemRB.SetVarAssoc(MovieWindow, TextAreaControl, "MovieIndex",0)
	GemRB.SetText(MovieWindow, PlayButton, 17318)
	GemRB.SetText(MovieWindow, CreditsButton, 15591)
	GemRB.SetText(MovieWindow, DoneButton, 11973)
	GemRB.SetEvent(MovieWindow, PlayButton, IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	GemRB.SetEvent(MovieWindow, CreditsButton, IE_GUI_BUTTON_ON_PRESS, "CreditsPress")
	GemRB.SetEvent(MovieWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "DonePress")
	GemRB.SetVisible(MovieWindow,1)


def PlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = GemRB.GetTableRowName(MoviesTable, i)
				GemRB.SetVisible (MovieWindow, 0)
				GemRB.PlayMovie(s)
				GemRB.SetVisible (MovieWindow, 1)
				return

			s = s - 1


def CreditsPress():
	GemRB.SetVisible (MovieWindow, 0)
	GemRB.PlayMovie("CREDITS")
	GemRB.SetVisible (MovieWindow, 1)


def DonePress():
	GemRB.UnloadWindow(MovieWindow)
	GemRB.SetNextScript("Start")


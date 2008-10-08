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
# $Id$


# Start.py - intro and main menu screens

###################################################

import GemRB
from GUICommonWindows import OpenWaitForDiscWindow

StartWindow = 0
QuitWindow = 0

def OnLoad():
	global StartWindow, QuitWindow

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ("BISLOGO")
		GemRB.PlayMovie ("TSRLOGO")
		GemRB.PlayMovie ("OPENING")

		GemRB.SetVar ("SkipIntroVideos", 1)

	GemRB.LoadWindowPack("START")
#quit subwindow
	QuitWindow = GemRB.LoadWindow(3)
	QuitTextArea = GemRB.GetControl(QuitWindow,0)
	CancelButton = GemRB.GetControl(QuitWindow, 2)
	ConfirmButton = GemRB.GetControl(QuitWindow, 1)
	GemRB.SetText(QuitWindow, QuitTextArea, 20582)
	GemRB.SetText(QuitWindow, CancelButton, 23789)
	GemRB.SetText(QuitWindow, ConfirmButton, 23787)
	GemRB.SetEvent(QuitWindow, ConfirmButton, IE_GUI_BUTTON_ON_PRESS, "ExitConfirmed")
	GemRB.SetEvent(QuitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "ExitCancelled")
	GemRB.SetButtonFlags(QuitWindow, CancelButton, IE_GUI_BUTTON_DEFAULT, OP_OR)

#main window
	StartWindow = GemRB.LoadWindow(0)
	NewLifeButton = GemRB.GetControl(StartWindow, 0)
	ResumeLifeButton = GemRB.GetControl(StartWindow, 2)
	ExitButton = GemRB.GetControl(StartWindow, 3)
	GemRB.SetEvent(StartWindow, NewLifeButton, IE_GUI_BUTTON_ON_PRESS, "NewLifePress")
	GemRB.SetEvent(StartWindow, ResumeLifeButton, IE_GUI_BUTTON_ON_PRESS, "ResumeLifePress")
	GemRB.SetEvent(StartWindow, ExitButton, IE_GUI_BUTTON_ON_PRESS, "ExitPress")

	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,415,640,30, "EXOFONT", "", 1)
	Label=GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, Label,GEMRB_VERSION)
	
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)

	GemRB.LoadMusicPL("Main.mus")
	return
	
def NewLifePress():
	GemRB.UnloadWindow(QuitWindow)
	GemRB.UnloadWindow(StartWindow)
	#to make difference between ingame change and new life
	GemRB.SetVar("PlayMode",0)
	GemRB.SetNextScript("NewLife")
	return

def ResumeLifePress():
	GemRB.UnloadWindow(QuitWindow)
	GemRB.UnloadWindow(StartWindow)
	#to make difference between ingame load and initial load
	GemRB.SetVar("PlayMode",0)
	GemRB.SetNextScript("GUILOAD")
	return

def ExitPress():
	GemRB.SetVisible(StartWindow,2)
	GemRB.SetVisible(QuitWindow,1)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def ExitCancelled():
	GemRB.SetVisible(QuitWindow, 0)
	GemRB.SetVisible(StartWindow, 1)
	return

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
	QuitWindow = GemRB.LoadWindowObject(3)
	QuitTextArea = QuitWindow.GetControl(0)
	CancelButton = QuitWindow.GetControl(2)
	ConfirmButton = QuitWindow.GetControl(1)
	QuitTextArea.SetText(20582)
	CancelButton.SetText(23789)
	ConfirmButton.SetText(23787)
	ConfirmButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExitConfirmed")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExitCancelled")
	CancelButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

#main window
	StartWindow = GemRB.LoadWindowObject(0)
	NewLifeButton = StartWindow.GetControl(0)
	ResumeLifeButton = StartWindow.GetControl(2)
	ExitButton = StartWindow.GetControl(3)
	NewLifeButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "NewLifePress")
	ResumeLifeButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ResumeLifePress")
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ExitPress")

	StartWindow.CreateLabel(0x0fff0000, 0,415,640,30, "EXOFONT", "", 1)
	Label=StartWindow.GetControl(0x0fff0000)
	Label.SetText(GEMRB_VERSION)
	
	QuitWindow.SetVisible(0)
	StartWindow.SetVisible(1)

	GemRB.LoadMusicPL("Main.mus")
	return
	
def NewLifePress():
	if QuitWindow:
		QuitWindow.Unload()
	if StartWindow:
		StartWindow.Unload()
	#to make difference between ingame change and new life
	GemRB.SetVar("PlayMode",0)
	GemRB.SetNextScript("NewLife")
	return

def ResumeLifePress():
	if QuitWindow:
		QuitWindow.Unload()
	if StartWindow:
		StartWindow.Unload()
	#to make difference between ingame load and initial load
	GemRB.SetVar("PlayMode",0)
	GemRB.SetNextScript("GUILOAD")
	return

def ExitPress():
	StartWindow.SetVisible(2)
	QuitWindow.SetVisible(1)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def ExitCancelled():
	QuitWindow.SetVisible(0)
	StartWindow.SetVisible(1)
	return

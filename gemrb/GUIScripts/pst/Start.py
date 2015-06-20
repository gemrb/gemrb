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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# Start.py - intro and main menu screens

###################################################

import GemRB
from GUIDefines import *

StartWindow = 0
QuitWindow = 0

def OnLoad():
	global StartWindow, QuitWindow

	skip_videos = GemRB.GetVar ("SkipIntroVideos")

	if not skip_videos:
		GemRB.PlayMovie ("BISLOGO", 1)
		GemRB.PlayMovie ("TSRLOGO", 1)
		GemRB.PlayMovie ("OPENING", 1)

		GemRB.SetVar ("SkipIntroVideos", 1)

#quit subwindow
	QuitWindow = GemRB.LoadWindow(3, "START")
	QuitTextArea = QuitWindow.GetControl(0)
	QuitTextArea.SetText(20582)
	ConfirmButton = QuitWindow.GetControl(1)
	ConfirmButton.SetText(23787)
	ConfirmButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, GemRB.Quit)
	ConfirmButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton = QuitWindow.GetControl(2)
	CancelButton.SetText(23789)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, StartWindow.Focus)
	CancelButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)

#main window
	StartWindow = GemRB.LoadWindow(0)
	NewLifeButton = StartWindow.GetControl(0)
	ResumeLifeButton = StartWindow.GetControl(2)
	ExitButton = StartWindow.GetControl(3)
	NewLifeButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NewLifePress)
	ResumeLifeButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ResumeLifePress)
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitWindow.Focus)
	ExitButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)

	StartWindow.CreateLabel(0x0fff0000, 0,415,640,30, "FONTDLG", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label=StartWindow.GetControl(0x0fff0000)
	Label.SetText(GEMRB_VERSION)
	
	StartWindow.Focus()

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
	

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
import GemRB
from GUIDefines import *

StartWindow = 0
QuitWindow = 0
ExitButton = 0
SinglePlayerButton = 0
MultiPlayerButton = 0
MoviesButton = 0

def OnLoad():
	global StartWindow, QuitWindow
	global ExitButton, MultiPlayerButton, MoviesButton, SinglePlayerButton

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ('BG4LOGO',1)
		GemRB.PlayMovie ('TSRLOGO',1)
		GemRB.PlayMovie ('BILOGO',1)
		GemRB.PlayMovie ('INFELOGO',1)
		GemRB.PlayMovie ('INTRO',1)
		GemRB.SetVar ("SkipIntroVideos", 1)

	GemRB.LoadWindowPack("START", 640, 480)

	#quit subwindow
	QuitWindow = GemRB.LoadWindow(3)
	QuitTextArea = QuitWindow.GetControl(0)
	CancelButton = QuitWindow.GetControl(2)
	ConfirmButton = QuitWindow.GetControl(1)
	QuitTextArea.SetText(19532)
	CancelButton.SetText(13727)
	ConfirmButton.SetText(15417)
	ConfirmButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ExitConfirmed)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ExitCancelled)
	ConfirmButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	#main window
	StartWindow = GemRB.LoadWindow(0)
	SinglePlayerButton = StartWindow.GetControl(0)
	MultiPlayerButton = StartWindow.GetControl(1)
	MoviesButton = StartWindow.GetControl(2)
	ExitButton = StartWindow.GetControl(3)

	BackToMain()
	
	GemRB.LoadMusicPL("Theme.mus")
	return
	
def SinglePlayerPress():
	
	SinglePlayerButton.SetText(13728)
	MultiPlayerButton.SetText(13729)
	MoviesButton.SetText(24110)
	ExitButton.SetText(15416)
	MultiPlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, LoadSingle)
	SinglePlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NewSingle)
	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MissionPack)
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackToMain)
	ExitButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)
	if GemRB.GetString(24110) == "": # TODO: better way to detect lack of mission pack?
		MoviesButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	return

def MultiPlayerPress():
	
	SinglePlayerButton.SetText(11825)
	MultiPlayerButton.SetText(20642)
	MoviesButton.SetText(15416)
	ExitButton.SetText("")
	SinglePlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, PregenPress)
	MultiPlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ConnectPress)
	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackToMain)
	MoviesButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, None)
	ExitButton.SetStatus(IE_GUI_BUTTON_DISABLED)
	ExitButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	return

def ConnectPress():
#well...
	return

def PregenPress():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetVar("PlayMode",0) #loadgame needs this hack
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetVar("PlayMode",-1)
	GemRB.SetNextScript("CharGen")
	return

def LoadSingle():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetVar("PlayMode",0)
	GemRB.SetVar("SaveDir",0)
	GemRB.SetNextScript("GUILOAD")
	return

def MissionPack():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetVar("PlayMode",1)
	GemRB.SetVar("SaveDir",1) #use mpsave for saved games
	GemRB.SetNextScript("GUILOAD")
	return

def NewSingle():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetVar("PlayMode",0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript("CharGen") #temporarily
	return

def ExitPress():
	StartWindow.SetVisible(WINDOW_INVISIBLE)
	QuitWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def MoviesPress():
#apparently the order is important
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	GemRB.SetNextScript("GUIMOVIE")
	return

def ExitCancelled():
	QuitWindow.SetVisible(WINDOW_INVISIBLE)
	StartWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def BackToMain():
	SinglePlayerButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	MultiPlayerButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	MoviesButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	ExitButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SinglePlayerButton.SetText(15413)
	MultiPlayerButton.SetText(15414)
	MoviesButton.SetText(15415)
	ExitButton.SetText(15417)
	SinglePlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, SinglePlayerPress)
	MultiPlayerButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MultiPlayerPress)
	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MoviesPress)
	ExitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ExitPress)
	MoviesButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	ExitButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_NAND)
	ExitButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	StartWindow.Focus()
	return

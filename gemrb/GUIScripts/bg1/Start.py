# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
import GemRB
from GUIDefines import *
from GameCheck import HasTOTSC

ExitButton = 0
SinglePlayerButton = 0
MultiPlayerButton = 0
MoviesButton = 0

def OnLoad():
	global ExitButton, MultiPlayerButton, MoviesButton, SinglePlayerButton

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ('BG4LOGO',1)
		GemRB.PlayMovie ('TSRLOGO',1)
		GemRB.PlayMovie ('BILOGO',1)
		GemRB.PlayMovie ('INFELOGO',1)
		GemRB.PlayMovie ('INTRO',1)
		GemRB.SetVar ("SkipIntroVideos", 1)

	#main window
	StartWindow = GemRB.LoadWindow (0, "START")
	SinglePlayerButton = StartWindow.GetControl(0)
	MultiPlayerButton = StartWindow.GetControl(1)
	MoviesButton = StartWindow.GetControl(2)
	ExitButton = StartWindow.GetControl(3)

	BackToMain()
	
	GemRB.LoadMusicPL("Theme.mus")
	return
	
def SinglePlayerPress():
	
	SinglePlayerButton.SetText(13728)
	MultiPlayerButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	MultiPlayerButton.SetText(13729)
	ExitButton.SetText(15416)
	MultiPlayerButton.OnPress (LoadSingle)
	SinglePlayerButton.OnPress (NewSingle)
	ExitButton.OnPress (BackToMain)
	ExitButton.MakeEscape()
	MoviesButton.SetText("")
	if HasTOTSC():
		MoviesButton.OnPress (MissionPack)
		MoviesButton.SetText(24110)
	else:
		MoviesButton.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		MoviesButton.SetStatus(IE_GUI_BUTTON_DISABLED)

	return

def LoadSingle():
	GemRB.SetVar("PlayMode",0)
	GemRB.SetToken ("SaveDir", "save")
	GemRB.SetNextScript("GUILOAD")
	return

def MissionPack():
	GemRB.SetVar("PlayMode",1)
	GemRB.SetToken ("SaveDir", "mpsave")
	GemRB.SetNextScript("GUILOAD")
	return

def NewSingle():
	GemRB.SetVar("PlayMode",0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript("CharGen") #temporarily
	return

def ExitPress():
	QuitWindow = GemRB.LoadWindow (3, "START")

	QuitTextArea = QuitWindow.GetControl (0)
	QuitTextArea.SetText (19532)

	CancelButton = QuitWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.OnPress (QuitWindow.Close)
	CancelButton.MakeEscape ()

	ConfirmButton = QuitWindow.GetControl (1)
	ConfirmButton.SetText (15417)
	ConfirmButton.OnPress (ExitConfirmed)
	ConfirmButton.MakeDefault ()

	QuitWindow.ShowModal (MODAL_SHADOW_GRAY)
	return
	
def ExitConfirmed():
	GemRB.Quit()
	return

def MoviesPress():
	GemRB.SetNextScript("GUIMOVIE")
	return
	
def BackToMain():
	SinglePlayerButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	MoviesButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	ExitButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SinglePlayerButton.SetText(15413)
	MultiPlayerButton.SetText(15414)
	MoviesButton.SetText(15415)
	ExitButton.SetText(15417)
	SinglePlayerButton.OnPress (SinglePlayerPress)
	MultiPlayerButton.SetState (IE_GUI_BUTTON_DISABLED)
	MoviesButton.OnPress (MoviesPress)
	ExitButton.OnPress (ExitPress)
	MoviesButton.SetFlags(IE_GUI_BUTTON_NORMAL, OP_SET)
	ExitButton.SetFlags(IE_GUI_BUTTON_NORMAL, OP_SET)
	ExitButton.MakeEscape()

	return

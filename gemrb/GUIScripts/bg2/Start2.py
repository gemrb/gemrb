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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#
#this is essentially Start.py from the SoA game, except for a very small change
import GemRB
import GameCheck

StartWindow = 0
TutorialWindow = 0
QuitWindow = 0
ExitButton = 0
SinglePlayerButton = 0
OptionsButton = 0
MultiPlayerButton = 0
MoviesButton = 0
BackButton = 0

def OnLoad():
	global StartWindow, TutorialWindow, QuitWindow
	global ExitButton, OptionsButton, MultiPlayerButton, MoviesButton, SinglePlayerButton, BackButton
	global SinglePlayerButton

	skip_videos = GemRB.GetVar ("SkipIntroVideos")

	GemRB.LoadWindowPack("START", 640, 480)
#tutorial subwindow
	if not GameCheck.IsBG2Demo():
		TutorialWindow = GemRB.LoadWindow (5)
		TextAreaControl = TutorialWindow.GetControl (1)
		CancelButton = TutorialWindow.GetControl (11)
		PlayButton = TutorialWindow.GetControl (10)
		TextAreaControl.SetText (44200)
		CancelButton.SetText (13727)
		PlayButton.SetText (33093)
		PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PlayPress)
		CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelTut)
		PlayButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
		CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	else:
		GemRB.SetFeature (GF_ALL_STRINGS_TAGGED, True)

#quit subwindow
	QuitWindow = GemRB.LoadWindow (3)
	QuitTextArea = QuitWindow.GetControl (0)
	CancelButton = QuitWindow.GetControl (2)
	ConfirmButton = QuitWindow.GetControl (1)
	QuitTextArea.SetText (19532)
	CancelButton.SetText (13727)
	ConfirmButton.SetText (15417)
	ConfirmButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitConfirmed)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitCancelled)
	ConfirmButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
#main window
	StartWindow = GemRB.LoadWindow (0)
	StartWindow.SetFrame ()
	#this is the ToB specific part of Start.py
	if GemRB.GetVar("oldgame")==1:
		if GameCheck.HasTOB():
			StartWindow.SetPicture("STARTOLD")
		if not skip_videos:
			GemRB.PlayMovie ("INTRO15F", 1)
	else:
		if not skip_videos:
			GemRB.PlayMovie ("INTRO", 1)

	#end ToB specific part
	SinglePlayerButton = StartWindow.GetControl (0)
	ExitButton = StartWindow.GetControl (3)
	OptionsButton = StartWindow.GetControl (4)
	MultiPlayerButton = StartWindow.GetControl (1)
	MoviesButton = StartWindow.GetControl (2)
	BackButton = StartWindow.GetControl (5)
	StartWindow.CreateLabel(0x0fff0000, 0,450,640,30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label=StartWindow.GetControl (0x0fff0000)
	Label.SetText (GEMRB_VERSION)
	if GameCheck.HasTOB():
		BackButton.SetState (IE_GUI_BUTTON_ENABLED)
		BackButton.SetText (15416)
	else:
		BackButton.SetState (IE_GUI_BUTTON_DISABLED)
		BackButton.SetText ("")
	SinglePlayerButton.SetState (IE_GUI_BUTTON_ENABLED)
	ExitButton.SetState (IE_GUI_BUTTON_ENABLED)
	OptionsButton.SetState (IE_GUI_BUTTON_ENABLED)
	if GameCheck.IsBG2Demo():
		MultiPlayerButton.SetState (IE_GUI_BUTTON_DISABLED)
		MoviesButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		MultiPlayerButton.SetState (IE_GUI_BUTTON_ENABLED)
		MultiPlayerButton.SetText (15414)
		MultiPlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MultiPlayerPress)

		MoviesButton.SetState (IE_GUI_BUTTON_ENABLED)
		MoviesButton.SetText (15415)
		MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MoviesPress)
	SinglePlayerButton.SetText (15413)
	ExitButton.SetText (15417)
	OptionsButton.SetText (13905)
	SinglePlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SinglePlayerPress)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitPress)
	OptionsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OptionsPress)
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, Restart)
	ExitButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	QuitWindow.SetVisible (WINDOW_INVISIBLE)
	if not GameCheck.IsBG2Demo():
		TutorialWindow.SetVisible (WINDOW_INVISIBLE)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	MusicTable = GemRB.LoadTable ("songlist")
	# the table has useless rownames, so we can't search for BG2Theme
	theme = MusicTable.GetValue ("33", "RESOURCE")
	GemRB.LoadMusicPL (theme, 1)
	return

def SinglePlayerPress():

	SinglePlayerButton.SetText (13728)
	SinglePlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NewSingle)
	
	MultiPlayerButton.SetText (13729)
	MultiPlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadSingle)
	MultiPlayerButton.SetState (IE_GUI_BUTTON_ENABLED)

	if not GameCheck.IsBG2Demo():
		if GemRB.GetVar("oldgame")==1:
			MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, Tutorial)
			MoviesButton.SetText (33093)
		else:
			MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ImportGame)
			MoviesButton.SetText (71175)

	ExitButton.SetText (15416)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BackToMain)
	
	OptionsButton.SetText ("")
	OptionsButton.SetState (IE_GUI_BUTTON_DISABLED)
	
	BackButton.SetText ("")
	BackButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def MultiPlayerPress():

	OptionsButton.SetText ("")
	SinglePlayerButton.SetText (20642)
	ExitButton.SetText (15416)
	MultiPlayerButton.SetText ("")
	MoviesButton.SetText (11825)
	MultiPlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
	SinglePlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ConnectPress)
	MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PregenPress)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BackToMain)
	MultiPlayerButton.SetState (IE_GUI_BUTTON_DISABLED)
	OptionsButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def ConnectPress():
#well...
	#GemRB.SetVar("PlayMode",2)
	return

def PregenPress():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	#do not start game after chargen
	GemRB.SetVar("PlayMode",-1) #will allow export
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("CharGen")
	return

def LoadSingle():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	if GemRB.GetVar ("oldgame") == 0:
		GemRB.SetVar ("PlayMode", 2)
		GemRB.SetVar ("SaveDir", 1)
	else:
		GemRB.SetVar ("PlayMode", 0)
		GemRB.SetVar ("SaveDir", 0)

	GemRB.SetNextScript ("GUILOAD")
	return

def NewSingle():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	if GemRB.GetVar ("oldgame") == 0:
		GemRB.SetVar ("PlayMode", 2)
		GemRB.SetVar ("SaveDir", 1)
	else:
		GemRB.SetVar ("PlayMode", 0)
		GemRB.SetVar ("SaveDir", 0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("CharGen")
	return

def ImportGame():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	#now this is tricky, we need to load old games, but set up the expansion
	GemRB.SetVar ("PlayMode", 0)
	GemRB.SetVar ("SaveDir", 0)
	GemRB.SetNextScript ("GUILOAD")
	return
	
def Tutorial():
	StartWindow.SetVisible (WINDOW_INVISIBLE)
	TutorialWindow.SetVisible (WINDOW_VISIBLE)
	return

def PlayPress():
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	GemRB.SetVar("PlayMode",1) #tutorial
	GemRB.SetVar("SaveDir",0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("CharGen")
	return

def CancelTut():
	TutorialWindow.SetVisible (WINDOW_INVISIBLE)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return

def ExitPress():
	StartWindow.SetVisible (WINDOW_INVISIBLE)
	QuitWindow.SetVisible (WINDOW_VISIBLE)
	return

def ExitConfirmed():
	GemRB.Quit()
	return

def OptionsPress():
#apparently the order is important
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	GemRB.SetNextScript ("StartOpt")
	return

def MoviesPress():
#apparently the order is important
	if StartWindow:
		StartWindow.Unload()
	if QuitWindow:
		QuitWindow.Unload()
	if TutorialWindow:
		TutorialWindow.Unload()
	GemRB.SetNextScript ("GUIMOVIE")
	return

def ExitCancelled():
	QuitWindow.SetVisible (WINDOW_INVISIBLE)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return

def BackToMain():
	SinglePlayerButton.SetState (IE_GUI_BUTTON_ENABLED)
	OptionsButton.SetState (IE_GUI_BUTTON_ENABLED)
	MultiPlayerButton.SetState (IE_GUI_BUTTON_ENABLED)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)
	SinglePlayerButton.SetText (15413)
	ExitButton.SetText (15417)
	OptionsButton.SetText (13905)
	if GameCheck.IsBG2Demo():
		MoviesButton.SetText ("")
		BackButton.SetText ("")
		BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		BackButton.SetState (IE_GUI_BUTTON_DISABLED)
		MultiPlayerButton.SetText ("")
		MultiPlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		MultiPlayerButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		MultiPlayerButton.SetText (15414)
		MultiPlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MultiPlayerPress)
		MoviesButton.SetText (15415)
		MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MoviesPress)
		BackButton.SetText (15416)

	SinglePlayerButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, SinglePlayerPress)
	ExitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExitPress)
	OptionsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OptionsPress)
	QuitWindow.SetVisible (WINDOW_INVISIBLE)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return

def Restart():
	StartWindow.Unload()
	QuitWindow.Unload()
	GemRB.SetNextScript ("Start")
	return


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

def OnLoad():
	StartWindow = GemRB.LoadWindow (0, "START")
	StartWindow.AddAlias("START2")
	
	if GameCheck.HasTOB() and GemRB.GetVar("oldgame") == 1 and not GameCheck.IsBG2EE ():
		StartWindow.SetBackground("STARTOLD")

	SinglePlayerButton = StartWindow.GetControl (0)
	ExitButton = StartWindow.GetControl (3)
	OptionsButton = StartWindow.GetControl (4)
	MultiPlayerButton = StartWindow.GetControl (1)
	MoviesButton = StartWindow.GetControl (2)
	BackButton = StartWindow.GetControl (5)

	Title = StartWindow.GetControl (6)
	Logo = StartWindow.GetControl (7)
	if Logo:
		# should be 2 for bp
		frame = 0 if GemRB.GetVar ("oldgame") else 1
		Title.SetSprites ("title", 0, frame, frame, 0, 0)
		Logo.SetSprites ("biglogo", 0, frame, frame, 0, 0)
		if frame == 1: # both tob and bp logos are shorter and not centered bams
			btnFrame = Logo.GetFrame ()
			Logo.SetPos (btnFrame["x"], btnFrame["y"] + 70)

	y = 450
	w = 640
	if GameCheck.IsBG2EE ():
		y = GemRB.GetSystemVariable (SV_HEIGHT) - 100
		w = GemRB.GetSystemVariable (SV_WIDTH)
	Label = StartWindow.CreateLabel(0x0fff0000, 0, y, w, 30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText (GemRB.Version)

	if GameCheck.HasTOB():
		BackButton.SetState (IE_GUI_BUTTON_ENABLED)
		BackButton.SetText (15416)
	else:
		BackButton.SetState (IE_GUI_BUTTON_DISABLED)
		BackButton.SetText ("")
	SinglePlayerButton.SetState (IE_GUI_BUTTON_ENABLED)
	ExitButton.SetState (IE_GUI_BUTTON_ENABLED)
	OptionsButton.SetState (IE_GUI_BUTTON_ENABLED)

	MultiPlayerButton.SetState (IE_GUI_BUTTON_DISABLED)
	if GameCheck.IsBG2Demo():
		MoviesButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		MultiPlayerButton.SetText (15414)
		MultiPlayerButton.OnPress (MultiPlayerPress)

		MoviesButton.SetState (IE_GUI_BUTTON_ENABLED)
		MoviesButton.SetText (15415)
		MoviesButton.OnPress (MoviesPress)
	SinglePlayerButton.SetText (15413)
	ExitButton.SetText (15417)
	OptionsButton.SetText (13905)
	SinglePlayerButton.OnPress (SinglePlayerPress)
	ExitButton.OnPress (ExitPress)
	OptionsButton.OnPress (OptionsPress)
	BackButton.OnPress (StartWindow.Close)
	ExitButton.MakeEscape()
	StartWindow.Focus ()
	
	GemRB.SetToken ("SaveDir", "save")
	return

def SinglePlayerPress():
	StartWindow = GemRB.GetView("START2")
	
	SinglePlayerButton = StartWindow.GetControl (0)
	SinglePlayerButton.SetText (13728)
	SinglePlayerButton.OnPress (NewSingle)
	
	MultiPlayerButton = StartWindow.GetControl (1)
	MultiPlayerButton.SetText (13729)
	MultiPlayerButton.OnPress (LoadSingle)
	MultiPlayerButton.SetState (IE_GUI_BUTTON_ENABLED)

	if not GameCheck.IsBG2Demo():
		Button = StartWindow.GetControl (2)
		if GemRB.GetVar("oldgame")==1:
			Button.OnPress (Tutorial)
			Button.SetText (33093)
		else:
			Button.OnPress (ImportGame)
			Button.SetText (71175)

	ExitButton = StartWindow.GetControl (3)
	ExitButton.SetText (15416)
	ExitButton.OnPress (Restart)
	
	OptionsButton = StartWindow.GetControl (4)
	OptionsButton.SetText ("")
	OptionsButton.SetState (IE_GUI_BUTTON_DISABLED)
	
	BackButton = StartWindow.GetControl (5)
	BackButton.SetText ("")
	BackButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def MultiPlayerPress():
	return

def LoadSingle():
	if GemRB.GetVar ("oldgame") == 0:
		GemRB.SetVar ("PlayMode", 2)
	else:
		GemRB.SetVar ("PlayMode", 0)

	GemRB.SetNextScript ("GUILOAD")
	return

def NewSingle():
	if GemRB.GetVar ("oldgame") == 0:
		GemRB.SetVar ("PlayMode", 2)
	else:
		GemRB.SetVar ("PlayMode", 0)
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("CharGen")
	return

def ImportGame():
	#now this is tricky, we need to load old games, but set up the expansion
	GemRB.SetVar ("PlayMode", 0)
	GemRB.SetNextScript ("GUILOAD")
	return
	
def Tutorial():
	#tutorial subwindow
	TutorialWindow = GemRB.LoadWindow (5, "START")
	TutorialWindow.ShowModal()
	TextAreaControl = TutorialWindow.GetControl (1)
	CancelButton = TutorialWindow.GetControl (11)
	PlayButton = TutorialWindow.GetControl (10)
	TextAreaControl.SetText (44200)
	CancelButton.SetText (13727)
	PlayButton.SetText (33093)
	PlayButton.OnPress (PlayPress)
	CancelButton.OnPress (TutorialWindow.Close)
	PlayButton.MakeDefault()
	CancelButton.MakeEscape()		

	return

def PlayPress():
	GemRB.SetVar("PlayMode",1) #tutorial
	GemRB.SetVar("Slot",1)
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("CharGen")
	return

def ExitPress():
	#quit subwindow
	QuitWindow = GemRB.LoadWindow (3, "START")
	Pos = QuitWindow.GetPos ()
	QuitWindow.SetPos (Pos[0] - 3, Pos[1] + 12)
	QuitTextArea = QuitWindow.GetControl (0)
	CancelButton = QuitWindow.GetControl (2)
	ConfirmButton = QuitWindow.GetControl (1)
	QuitTextArea.SetText (19532)
	CancelButton.SetText (13727)
	ConfirmButton.SetText (15417)
	ConfirmButton.OnPress (lambda: GemRB.Quit())
	CancelButton.OnPress (QuitWindow.Close)
	ConfirmButton.MakeDefault()
	CancelButton.MakeEscape()
	return

def OptionsPress():
	GemRB.SetNextScript ("StartOpt")
	return

def MoviesPress():
	GemRB.SetNextScript ("GUIMOVIE")
	return

def Restart():
	StartWindow = GemRB.GetView("START2")
	StartWindow.Close()
	GemRB.SetNextScript ("Start2") # can't just call OnLoad because that won't destroy the existing variables
	return

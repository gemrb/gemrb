# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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
import GameCheck
from GUIDefines import *
from ie_restype import *

StartWindow = None
JoinGameButton = 0

def OnLoad ():
	global JoinGameButton

	GemRB.SetVar("ExpansionGame", 0)

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ("BISLOGO", 1)
		if GameCheck.HasHOW():
			GemRB.PlayMovie ("WOTC", 1)
		else:
			GemRB.PlayMovie ("TSRLOGO", 1)
		GemRB.PlayMovie("INTRO", 1)
		GemRB.SetVar ("SkipIntroVideos", 1)

	if GameCheck.HasHOW():
		GemRB.SetMasterScript("BALDUR","WORLDMAP","EXPMAP")
	else:
		GemRB.SetMasterScript("BALDUR","WORLDMAP")

#main window
	global StartWindow
	StartWindow = GemRB.LoadWindow (0, "GUICONN")
	ProtocolButton = StartWindow.GetControl (0x00)
	CreateGameButton = StartWindow.GetControl (0x02)
	LoadGameButton = StartWindow.GetControl (0x07)
	JoinGameButton = StartWindow.GetControl (0x03)
	MoviesButton = StartWindow.GetControl (0x08)
	QuitGameButton = StartWindow.GetControl (0x01)
	VersionLabel = StartWindow.CreateLabel (0x0fff0000, 0, 0, 640, 30, "REALMS2", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	VersionLabel.SetText (GemRB.Version)
	VersionLabel.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
	ProtocolButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CreateGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	LoadGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	MoviesButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	QuitGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	LastProtocol = GemRB.GetVar ("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText (15413)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	elif LastProtocol == 1:
		ProtocolButton.SetText (13967)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	elif LastProtocol == 2:
		ProtocolButton.SetText (13968)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	CreateGameButton.SetText (13963)
	LoadGameButton.SetText (13729)
	JoinGameButton.SetText (13964)
	MoviesButton.SetText (15415)
	QuitGameButton.SetText (13731)
	QuitGameButton.OnPress (QuitPress)
	QuitGameButton.MakeEscape ()
	ProtocolButton.OnPress (ProtocolPress)
	MoviesButton.OnPress (MoviesPress)
	LoadGameButton.OnPress (LoadPress)
	CreateGameButton.OnPress (CreatePress)
	StartWindow.Focus ()
	GemRB.LoadMusicPL("Theme.mus",1)
	
	StartWindow.OnFocus (RefreshProtocol)
	
	return

def ProtocolPress ():
	ProtocolWindow = GemRB.LoadWindow (1, "GUICONN")
	
	#Disabling Unused Buttons in this Window
	Button = ProtocolWindow.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl (3)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl (9)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	
	SinglePlayerButton = ProtocolWindow.GetControl (10)
	SinglePlayerButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SinglePlayerButton.SetText (15413)
	
	IPXButton = ProtocolWindow.GetControl (0)
	IPXButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	IPXButton.SetText (13967)
	
	TCPIPButton = ProtocolWindow.GetControl (1)
	TCPIPButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	TCPIPButton.SetText (13968)
	
	SinglePlayerButton.SetVarAssoc ("Last Protocol Used", 0)
	IPXButton.SetVarAssoc ("Last Protocol Used", 1)
	TCPIPButton.SetVarAssoc ("Last Protocol Used", 2)
	
	TextArea = ProtocolWindow.GetControl (7)
	TextArea.SetText (11316)
	
	DoneButton = ProtocolWindow.GetControl (6)
	DoneButton.SetText (11973)
	DoneButton.OnPress (ProtocolWindow.Close)
	DoneButton.MakeEscape()
	
	ProtocolWindow.ShowModal (1)
	return
	
def RefreshProtocol (win):
	ProtocolButton = win.GetControl (0)
	
	LastProtocol = GemRB.GetVar ("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText (15413)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_DISABLED)
	elif LastProtocol == 1:
		ProtocolButton.SetText (13967)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)
	elif LastProtocol == 2:
		ProtocolButton.SetText (13968)
		JoinGameButton.SetStatus (IE_GUI_BUTTON_ENABLED)

	return
	
def CreatePress ():
	global GameTypeWindow, ExpansionType

	if not GameCheck.HasHOW():
		GameTypeReallyDonePress()

	GameTypeWindow = GemRB.LoadWindow (24, "GUICONN")

	CancelButton = GameTypeWindow.GetControl (1)
	CancelButton.SetText (13727)
	CancelButton.OnPress (GameTypeWindow.Close)
	CancelButton.MakeEscape()

	DoneButton = GameTypeWindow.GetControl (2)
	DoneButton.SetText (11973)
	DoneButton.OnPress (lambda: GameTypeDonePress(GameTypeWindow))
	DoneButton.MakeDefault()

	FullGameButton = GameTypeWindow.GetControl (4)
	FullGameButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FullGameButton.OnPress (lambda btn: GameTypeChange (btn, False))
	FullGameButton.SetText (24869)

	ExpansionGameButton = GameTypeWindow.GetControl (5)
	ExpansionGameButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	ExpansionGameButton.OnPress (lambda btn: GameTypeChange (btn, True))
	ExpansionGameButton.SetText (24871)

	ExpansionGameButton.SetVarAssoc ("ExpansionGame", 1)
	FullGameButton.SetVarAssoc ("ExpansionGame", 0)

	GameTypeChange (None, False)
	GameTypeWindow.ShowModal (1)
	return

def GameTypeChange(btn, isExpansion):
	GameTypeTextArea = GameTypeWindow.GetControl(3)
	if isExpansion:
		GameTypeTextArea.SetText (24872)
	else:
		GameTypeTextArea.SetText (24870)
	return

def GameTypeDonePress(win):
	#todo: importing team members from final save (string 26317)?

	ExpansionGame = GemRB.GetVar("ExpansionGame")
	if ExpansionGame == 0: #start in Easthaven
		win.Close()
		GameTypeReallyDonePress()
	elif ExpansionGame == 1: #show a warning message first
		GameType2Window = GemRB.LoadWindow (25, "GUICONN")

		TextArea = GameType2Window.GetControl(0)
		TextArea.SetText(26318)

		def confirm():
			win.Close()
			GameType2Window.Close()
			GameTypeReallyDonePress()

		YesButton = GameType2Window.GetControl (1)
		YesButton.SetText (13912)
		YesButton.OnPress (confirm)

		NoButton = GameType2Window.GetControl (2)
		NoButton.SetText (13913)
		NoButton.OnPress (GameType2Window.Close)
		NoButton.MakeEscape()

		def cancel():
			win.Close();
			GameType2Window.Close()

		CancelButton = GameType2Window.GetControl (3)
		CancelButton.SetText (13727)
		CancelButton.OnPress (cancel)
		CancelButton.MakeEscape()

		GameType2Window.ShowModal(1)

def GameTypeReallyDonePress():
	StartWindow.Close()
	GemRB.LoadGame(None)
	GemRB.SetNextScript ("PartyFormation")

def LoadPress ():
	GemRB.SetNextScript ("GUILOAD")
	return

def MoviesPress ():
	GemRB.SetNextScript ("GUIMOVIE")
	return
	
def QuitPress ():
	QuitWindow = GemRB.LoadWindow (22, "GUICONN")
	CancelButton = QuitWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.OnPress (QuitWindow.Close)
	CancelButton.MakeEscape()

	QuitButton = QuitWindow.GetControl (1)
	QuitButton.SetText (15417)
	QuitButton.OnPress (lambda: GemRB.Quit())
	QuitButton.MakeDefault()

	TextArea = QuitWindow.GetControl (0)
	TextArea.SetText (19532)
	QuitWindow.ShowModal (1)
	return

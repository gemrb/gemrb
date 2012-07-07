# -*-python-*-
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
from GUIDefines import *
from ie_restype import *
import GUICommon

StartWindow = 0
JoinGameButton = 0
ProtocolWindow = 0
GameTypeWindow = 0
GameType2Window = 0
ExpansionGame = 0
QuitWindow = 0

def OnLoad ():
	global StartWindow, JoinGameButton

	GemRB.SetVar("ExpansionGame", 0)

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ("BISLOGO", 1)
		if GUICommon.HasHOW():
			GemRB.PlayMovie ("WOTC", 1)
		else:
			GemRB.PlayMovie ("TSRLOGO", 1)
		GemRB.PlayMovie("INTRO", 1)
		GemRB.SetVar ("SkipIntroVideos", 1)

	if GUICommon.HasHOW():
		GemRB.SetMasterScript("BALDUR","WORLDMAP","EXPMAP")
	else:
		GemRB.SetMasterScript("BALDUR","WORLDMAP")

	# Find proper window border for higher resolutions
	screen_width = GemRB.GetSystemVariable (SV_WIDTH)
	screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
	if GemRB.HasResource ("STON08L", RES_MOS):
		if screen_width == 800:
			GemRB.LoadWindowFrame ("STON08L", "STON08R", "STON08T", "STON08B")
		elif screen_width == 1024:
			GemRB.LoadWindowFrame ("STON10L", "STON10R", "STON10T", "STON10B")

	GemRB.LoadWindowPack("GUICONN", 640, 480)

#main window
	StartWindow = GemRB.LoadWindow (0)
	StartWindow.SetFrame ()
	ProtocolButton = StartWindow.GetControl (0x00)
	CreateGameButton = StartWindow.GetControl (0x02)
	LoadGameButton = StartWindow.GetControl (0x07)
	JoinGameButton = StartWindow.GetControl (0x03)
	MoviesButton = StartWindow.GetControl (0x08)
	QuitGameButton = StartWindow.GetControl (0x01)
	StartWindow.CreateLabel (0x0fff0000, 0,0,800,30, "REALMS2", "", 1)
	VersionLabel = StartWindow.GetControl (0x0fff0000)
	VersionLabel.SetText (GEMRB_VERSION)
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
	QuitGameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitPress)
	ProtocolButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProtocolPress)
	MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, MoviesPress)
	LoadGameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadPress)
	CreateGameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CreatePress)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	GemRB.LoadMusicPL("Theme.mus",1)
	return

def ProtocolPress ():
	global StartWindow, ProtocolWindow
	StartWindow.SetVisible (WINDOW_INVISIBLE)
	ProtocolWindow = GemRB.LoadWindow (1)
	
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
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ProtocolDonePress)
	DoneButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	
	ProtocolWindow.ShowModal (1)
	return
	
def ProtocolDonePress ():
	global StartWindow, ProtocolWindow, JoinGameButton
	if ProtocolWindow:
		ProtocolWindow.Unload ()
		ProtocolWindow = None
	
	ProtocolButton = StartWindow.GetControl (0x00)
	
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
	
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return
	
def CreatePress ():
	global StartWindow, GameTypeWindow, ExpansionType

	if not GUICommon.HasHOW():
		GameTypeReallyDonePress()

	GameTypeWindow = GemRB.LoadWindow (24)

	CancelButton = GameTypeWindow.GetControl (1)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGameTypePress)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton = GameTypeWindow.GetControl (2)
	DoneButton.SetText (11973)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GameTypeDonePress)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	FullGameButton = GameTypeWindow.GetControl (4)
	FullGameButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	FullGameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GameTypeButtonPress)
	FullGameButton.SetText (24869)

	ExpansionGameButton = GameTypeWindow.GetControl (5)
	ExpansionGameButton.SetFlags (IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	ExpansionGameButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GameTypeButtonPress)
	ExpansionGameButton.SetText (24871)

	FullGameButton.SetVarAssoc ("ExpansionGame", 0)
	ExpansionGameButton.SetVarAssoc ("ExpansionGame", 1)

	GameTypeButtonPress()

	GameTypeWindow.ShowModal (1)
	return

def GameTypeButtonPress():
	global GameTypeWindow, ExpansionGame

	ExpansionGame=GemRB.GetVar("ExpansionGame")
	GameTypeTextArea = GameTypeWindow.GetControl(3)
	if ExpansionGame == 0:
	      GameTypeTextArea.SetText (24870)
	elif ExpansionGame == 1:
	      GameTypeTextArea.SetText (24872)
	return

def GameTypeDonePress():
	#todo: importing team members from final save (string 26317)?
	global StartWindow, GameTypeWindow, GameType2Window

	if GameTypeWindow:
		GameTypeWindow.Unload()
		GameTypeWindow = None

	if ExpansionGame == 0: #start in Easthaven
		GameTypeReallyDonePress()
	elif ExpansionGame == 1: #show a warning message first
		GameType2Window = GemRB.LoadWindow (25)

		TextArea = GameType2Window.GetControl(0)
		TextArea.SetText(26318)

		YesButton = GameType2Window.GetControl (1)
		YesButton.SetText (13912)
		YesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GameTypeReallyDonePress)

		NoButton = GameType2Window.GetControl (2)
		NoButton.SetText (13913)
		NoButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGameTypePress)
		NoButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

		CancelButton = GameType2Window.GetControl (3)
		CancelButton.SetText (13727)
		CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGameTypePress)
		CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

		GameType2Window.ShowModal(1)

def GameTypeReallyDonePress():
	global StartWindow, GameTypeWindow, GameType2Window

	if GameType2Window:
		GameType2Window.Unload()
		GameType2Window = None
	if StartWindow:
		StartWindow.Unload ()
		StartWindow = None

	GemRB.LoadGame(None)
	GemRB.SetNextScript ("PartyFormation")

def QuitGameTypePress ():
	global StartWindow, GameTypeWindow, GameType2Window
	if GameType2Window:
		GameType2Window.Unload ()
		GameType2Window = None
	if GameTypeWindow:
		GameTypeWindow.Unload ()
		GameTypeWindow = None
	GemRB.SetVar("ExpansionGame", 0)
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return

def LoadPress ():
	global StartWindow
	if StartWindow:
		StartWindow.Unload ()
		StartWindow = None
	GemRB.SetNextScript ("GUILOAD")
	return

def MoviesPress ():
	global StartWindow
	if StartWindow:
		StartWindow.Unload ()
		StartWindow = None
	GemRB.SetNextScript ("GUIMOVIE")
	return
	
def QuitPress ():
	global StartWindow, QuitWindow
	StartWindow.SetVisible (WINDOW_INVISIBLE)
	QuitWindow = GemRB.LoadWindow (22)
	CancelButton = QuitWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitCancelPress)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	QuitButton = QuitWindow.GetControl (1)
	QuitButton.SetText (15417)
	QuitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitQuitPress)
	QuitButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextArea = QuitWindow.GetControl (0)
	TextArea.SetText (19532)
	QuitWindow.ShowModal (1)
	return
	
def QuitCancelPress ():
	global StartWindow, QuitWindow
	if QuitWindow:
		QuitWindow.Unload ()
		QuitWindow = None
	StartWindow.SetVisible (WINDOW_VISIBLE)
	return
	
def QuitQuitPress ():
	GemRB.Quit ()
	return

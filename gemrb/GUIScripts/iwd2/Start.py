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
import GemRB
import LoadScreen
from GUIDefines import *
from datetime import datetime

QuickLoadSlot = 0
StartWindow = None;

def OnLoad():
	global StartWindow, QuickLoadSlot

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ('BISLOGO', 1)
		GemRB.PlayMovie ('WOTC', 1)
		GemRB.PlayMovie ('NVIDIA', 1)
		GemRB.PlayMovie ('INTRO', 1)
		GemRB.SetVar ("SkipIntroVideos", 1)

#main window
	StartWindow = GemRB.LoadWindow(0, "GUICONN")
	StartWindow.AddAlias ("STARTWIN", 0, True)
	time = datetime.now()

	# IWD2 has some nice background for the night
	if time.hour >= 18 or time.hour <= 6:
		StartWindow.SetBackground ("STARTN");

		AnimButton = StartWindow.GetControl (0xfff0001)
		if not AnimButton:
			AnimButton = StartWindow.CreateButton (0xfff0001, 57, 333, 100, 100);
		AnimButton.SetAnimation ("MMTRCHB")
		AnimButton.SetState (IE_GUI_BUTTON_LOCKED)
		AnimButton.SetFlags (IE_GUI_BUTTON_CENTER_PICTURES, OP_OR)

	ProtocolButton = StartWindow.GetControl(0x00)
	NewGameButton = StartWindow.GetControl(0x02)
	LoadGameButton = StartWindow.GetControl(0x07)
	QuickLoadButton = StartWindow.GetControl(0x03)
	JoinGameButton = StartWindow.GetControl(0x0B)
	OptionsButton = StartWindow.GetControl(0x08)
	QuitGameButton = StartWindow.GetControl(0x01)
	VersionLabel = StartWindow.GetControl (0xfff0000)
	if not VersionLabel:
		VersionLabel = StartWindow.CreateLabel (0x0fff0000, 0,0,800,30, "REALMS2", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	VersionLabel.SetText(GemRB.Version)
	ProtocolButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	NewGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	LoadGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)

	GemRB.SetToken ("SaveDir", "mpsave")
	Games=GemRB.GetSaveGames()

	#looking for the quicksave
	EnableQuickLoad = IE_GUI_BUTTON_DISABLED
	for Game in Games:
		Slotname = Game.GetSaveID()
		# quick save is 1
		if Slotname == 1:
			EnableQuickLoad = IE_GUI_BUTTON_ENABLED
			QuickLoadSlot = Game
			break

	QuickLoadButton.SetStatus(EnableQuickLoad)
	JoinGameButton.SetStatus(IE_GUI_BUTTON_DISABLED)
	OptionsButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	QuitGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText(15413)
	elif LastProtocol == 1:
		ProtocolButton.SetText(13967)
	elif LastProtocol == 2:
		ProtocolButton.SetText(13968)
	NewGameButton.SetText(13963)
	LoadGameButton.SetText(13729)
	QuickLoadButton.SetText(33508)
	JoinGameButton.SetText(13964)
	OptionsButton.SetText(13905)
	QuitGameButton.SetText(13731)
	QuitGameButton.MakeEscape()
	NewGameButton.OnPress (NewGamePress)
	QuitGameButton.OnPress (QuitPress)
	ProtocolButton.OnPress (ProtocolPress)
	OptionsButton.OnPress (OptionsPress)
	LoadGameButton.OnPress (LoadPress)
	QuickLoadButton.OnPress (QuickLoadPress)
	StartWindow.Focus()
	GemRB.LoadMusicPL("Theme.mus")
	
	StartWindow.SetAction(RefreshProtocol, ACTION_WINDOW_FOCUS_GAINED)

	return

def ProtocolPress():
	ProtocolWindow = GemRB.LoadWindow(1, "GUICONN")

	#Disabling Unused Buttons in this Window
	Button = ProtocolWindow.GetControl(2)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl(3)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = ProtocolWindow.GetControl(9)
	Button.SetState(IE_GUI_BUTTON_DISABLED)
	Button.SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	SinglePlayerButton = ProtocolWindow.GetControl(10)
	SinglePlayerButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	SinglePlayerButton.SetText(15413)

	IPXButton = ProtocolWindow.GetControl(0)
	IPXButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	IPXButton.SetText(13967)

	TCPIPButton = ProtocolWindow.GetControl(1)
	TCPIPButton.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	TCPIPButton.SetText(13968)

	SinglePlayerButton.SetVarAssoc("Last Protocol Used", 0)
	IPXButton.SetVarAssoc("Last Protocol Used", 1)
	TCPIPButton.SetVarAssoc("Last Protocol Used", 2)

	TextArea = ProtocolWindow.GetControl(7)
	TextArea.SetText(11316)

	DoneButton = ProtocolWindow.GetControl(6)
	DoneButton.SetText(11973)
	DoneButton.OnPress (ProtocolWindow.Close)
	DoneButton.MakeEscape()

	ProtocolWindow.Focus()
	return

def RefreshProtocol(win):
	ProtocolButton = win.GetControl(0)

	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText(15413)
	elif LastProtocol == 1:
		ProtocolButton.SetText(13967)
	elif LastProtocol == 2:
		ProtocolButton.SetText(13968)

	return

def LoadPress():
	GemRB.SetNextScript("GUILOAD")
	return

def QuickLoadPress():
	global QuickLoadSlot

	LoadScreen.StartLoadScreen()
	GemRB.LoadGame(QuickLoadSlot) # load & start game
	GemRB.EnterGame()
	return

def OptionsPress():
	GemRB.SetNextScript("Options")
	return

def QuitPress():
	QuitWindow = GemRB.LoadWindow(22, "GUICONN")
	CancelButton = QuitWindow.GetControl(2)
	CancelButton.OnPress (QuitWindow.Close)
	CancelButton.MakeEscape()

	QuitButton = QuitWindow.GetControl(1)
	QuitButton.OnPress (lambda: GemRB.Quit())
	QuitButton.MakeDefault()

	TextArea = QuitWindow.GetControl(0)
	CancelButton.SetText(13727)
	QuitButton.SetText(15417)
	TextArea.SetText(19532)
	QuitWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def NewGamePress():
	GemRB.SetNextScript("SPParty")
	return

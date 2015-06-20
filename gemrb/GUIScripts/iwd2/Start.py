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

StartWindow = 0
ProtocolWindow = 0
QuitWindow = 0
QuickLoadSlot = 0

def OnLoad():
	global StartWindow, QuickLoadSlot

#main window
	StartWindow = GemRB.LoadWindow(0, "GUICONN")
	ProtocolButton = StartWindow.GetControl(0x00)
	NewGameButton = StartWindow.GetControl(0x02)
	LoadGameButton = StartWindow.GetControl(0x07)
	QuickLoadButton = StartWindow.GetControl(0x03)
	JoinGameButton = StartWindow.GetControl(0x0B)
	OptionsButton = StartWindow.GetControl(0x08)
	QuitGameButton = StartWindow.GetControl(0x01)
	StartWindow.CreateLabel(0x0fff0000, 0,0,800,30, "REALMS2", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	VersionLabel = StartWindow.GetControl(0x0fff0000)
	VersionLabel.SetText(GEMRB_VERSION)
	ProtocolButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	NewGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	LoadGameButton.SetStatus(IE_GUI_BUTTON_ENABLED)

	GemRB.SetVar("SaveDir",1)
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
	QuitGameButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)
	NewGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NewGamePress)
	QuitGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitPress)
	ProtocolButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ProtocolPress)
	OptionsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, OptionsPress)
	LoadGameButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, LoadPress)
	QuickLoadButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuickLoadPress)
	StartWindow.SetVisible(WINDOW_VISIBLE)
	GemRB.LoadMusicPL("Theme.mus")

	return

def ProtocolPress():
	global StartWindow, ProtocolWindow
	#StartWindow.Unload()
	StartWindow.SetVisible(WINDOW_INVISIBLE)
	ProtocolWindow = GemRB.LoadWindow(1)

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
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, ProtocolDonePress)
	DoneButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	ProtocolWindow.SetVisible(WINDOW_VISIBLE)
	return

def ProtocolDonePress():
	global StartWindow, ProtocolWindow
	if ProtocolWindow:
		ProtocolWindow.Unload()

	ProtocolButton = StartWindow.GetControl(0x00)

	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		ProtocolButton.SetText(15413)
	elif LastProtocol == 1:
		ProtocolButton.SetText(13967)
	elif LastProtocol == 2:
		ProtocolButton.SetText(13968)

	StartWindow.SetVisible(WINDOW_VISIBLE)
	return

def LoadPress():
	global StartWindow

	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("GUILOAD")
	return

def QuickLoadPress():
	global StartWindow, QuickLoadSlot

	LoadScreen.StartLoadScreen()
	GemRB.LoadGame(QuickLoadSlot) # load & start game
	GemRB.EnterGame()
	return

def OptionsPress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("Options")
	return

def QuitPress():
	global StartWindow, QuitWindow
	StartWindow.SetVisible(WINDOW_INVISIBLE)
	QuitWindow = GemRB.LoadWindow(22)
	CancelButton = QuitWindow.GetControl(2)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitCancelPress)
	CancelButton.SetFlags(IE_GUI_BUTTON_CANCEL, OP_OR)

	QuitButton = QuitWindow.GetControl(1)
	QuitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, QuitQuitPress)
	QuitButton.SetFlags(IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextArea = QuitWindow.GetControl(0)
	CancelButton.SetText(13727)
	QuitButton.SetText(15417)
	TextArea.SetText(19532)
	QuitWindow.SetVisible(WINDOW_VISIBLE)
	return

def NewGamePress():
	global StartWindow
	if StartWindow:
		StartWindow.Unload()
	GemRB.SetNextScript("SPParty")
	return

def QuitCancelPress():
	global StartWindow, QuitWindow
	if QuitWindow:
		QuitWindow.Unload()
	StartWindow.SetVisible(WINDOW_VISIBLE)
	return

def QuitQuitPress():
	GemRB.Quit()
	return

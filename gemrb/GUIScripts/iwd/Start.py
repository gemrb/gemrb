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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/iwd/Start.py,v 1.9 2005/03/20 16:24:22 avenger_teambg Exp $


# Start.py - intro and main menu screens

###################################################

import GemRB

StartWindow = 0
JoinGameButton = 0
ProtocolWindow = 0
QuitWindow = 0

def OnLoad():
	global StartWindow, JoinGameButton

	skip_videos = GemRB.GetVar ("SkipIntroVideos")
	if not skip_videos:
		GemRB.PlayMovie ('BISLOGO')
		GemRB.PlayMovie ('WOTC')
		GemRB.PlayMovie ('INTRO')

		GemRB.SetVar ("SkipIntroVideos", 1)

	# Find proper window border for higher resolutions
	screen_width = GemRB.GetSystemVariable (SV_WIDTH)
	screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
	if screen_width == 800:
		GemRB.LoadWindowFrame("STON08L", "STON08R", "STON08T", "STON08B")
	elif screen_width == 1024:
		GemRB.LoadWindowFrame("STON10L", "STON10R", "STON10T", "STON10B")

	GemRB.LoadWindowPack("GUICONN", 640, 480)

#main window
	StartWindow = GemRB.LoadWindow(0)
	GemRB.SetWindowFrame(StartWindow)
	#GemRB.SetWindowSize(StartWindow, 800, 600)
	ProtocolButton = GemRB.GetControl(StartWindow, 0x00)
	CreateGameButton = GemRB.GetControl(StartWindow, 0x02)
	LoadGameButton = GemRB.GetControl(StartWindow, 0x07)
	JoinGameButton = GemRB.GetControl(StartWindow, 0x03)
	MoviesButton = GemRB.GetControl(StartWindow, 0x08)
	QuitGameButton = GemRB.GetControl(StartWindow, 0x01)
	GemRB.CreateLabel(StartWindow, 0x0fff0000, 0,0,800,30, "REALMS2", "", 1)
	VersionLabel = GemRB.GetControl(StartWindow, 0x0fff0000)
	GemRB.SetText(StartWindow, VersionLabel, GEMRB_VERSION)
	GemRB.SetControlStatus(StartWindow, ProtocolButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, CreateGameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, LoadGameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, MoviesButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetControlStatus(StartWindow, QuitGameButton, IE_GUI_BUTTON_ENABLED)
	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		GemRB.SetText(StartWindow, ProtocolButton, 15413)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_DISABLED)
	elif LastProtocol == 1:
		GemRB.SetText(StartWindow, ProtocolButton, 13967)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_ENABLED)
	elif LastProtocol == 2:
		GemRB.SetText(StartWindow, ProtocolButton, 13968)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_ENABLED)
	GemRB.SetText(StartWindow, CreateGameButton, 13963)
	GemRB.SetText(StartWindow, LoadGameButton, 13729)
	GemRB.SetText(StartWindow, JoinGameButton, 13964)
	GemRB.SetText(StartWindow, MoviesButton, 15415)
	GemRB.SetText(StartWindow, QuitGameButton, 13731)
	GemRB.SetEvent(StartWindow, QuitGameButton, IE_GUI_BUTTON_ON_PRESS, "QuitPress")
	GemRB.SetEvent(StartWindow, ProtocolButton, IE_GUI_BUTTON_ON_PRESS, "ProtocolPress")
	GemRB.SetEvent(StartWindow, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	GemRB.SetEvent(StartWindow, LoadGameButton, IE_GUI_BUTTON_ON_PRESS, "LoadPress")
	GemRB.SetEvent(StartWindow, CreateGameButton, IE_GUI_BUTTON_ON_PRESS, "CreatePress")
	GemRB.SetVisible(StartWindow, 1)
	GemRB.LoadMusicPL("Theme.mus")
	return

def ProtocolPress():
	global StartWindow, ProtocolWindow
	#GemRB.UnloadWindow(StartWindow)
	GemRB.SetVisible(StartWindow, 0)
	ProtocolWindow = GemRB.LoadWindow(1)
	
	#Disabling Unused Buttons in this Window
	Button = GemRB.GetControl(ProtocolWindow, 2)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = GemRB.GetControl(ProtocolWindow, 3)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	Button = GemRB.GetControl(ProtocolWindow, 9)
	GemRB.SetButtonState(ProtocolWindow, Button, IE_GUI_BUTTON_DISABLED)
	GemRB.SetButtonFlags(ProtocolWindow, Button, IE_GUI_BUTTON_NO_IMAGE, OP_OR)
	
	SinglePlayerButton = GemRB.GetControl(ProtocolWindow, 10)
	GemRB.SetButtonFlags(ProtocolWindow, SinglePlayerButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, SinglePlayerButton, 15413)
	
	IPXButton = GemRB.GetControl(ProtocolWindow, 0)
	GemRB.SetButtonFlags(ProtocolWindow, IPXButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, IPXButton, 13967)
	
	TCPIPButton = GemRB.GetControl(ProtocolWindow, 1)
	GemRB.SetButtonFlags(ProtocolWindow, TCPIPButton, IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	GemRB.SetText(ProtocolWindow, TCPIPButton, 13968)
	
	GemRB.SetVarAssoc(ProtocolWindow, SinglePlayerButton, "Last Protocol Used", 0)
	GemRB.SetVarAssoc(ProtocolWindow, IPXButton, "Last Protocol Used", 1)
	GemRB.SetVarAssoc(ProtocolWindow, TCPIPButton, "Last Protocol Used", 2)
	
	TextArea = GemRB.GetControl(ProtocolWindow, 7)
	GemRB.SetText(ProtocolWindow, TextArea, 11316)
	
	DoneButton = GemRB.GetControl(ProtocolWindow, 6)
	GemRB.SetText(ProtocolWindow, DoneButton, 11973)
	GemRB.SetEvent(ProtocolWindow, DoneButton, IE_GUI_BUTTON_ON_PRESS, "ProtocolDonePress")
	
	GemRB.SetVisible(ProtocolWindow, 1)
	return
	
def ProtocolDonePress():
	global StartWindow, ProtocolWindow, JoinGameButton
	GemRB.UnloadWindow(ProtocolWindow)
	
	ProtocolButton = GemRB.GetControl(StartWindow, 0x00)
	
	LastProtocol = GemRB.GetVar("Last Protocol Used")
	if LastProtocol == 0:
		GemRB.SetText(StartWindow, ProtocolButton, 15413)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_DISABLED)
	elif LastProtocol == 1:
		GemRB.SetText(StartWindow, ProtocolButton, 13967)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_ENABLED)
	elif LastProtocol == 2:
		GemRB.SetText(StartWindow, ProtocolButton, 13968)
		GemRB.SetControlStatus(StartWindow, JoinGameButton, IE_GUI_BUTTON_ENABLED)
	
	GemRB.SetVisible(StartWindow, 1)
	return
	
def CreatePress():
	global StartWindow
	GemRB.UnloadWindow(StartWindow)
	GemRB.LoadGame(-1)
	GemRB.SetNextScript("PartyFormation")
	return

def LoadPress():
	global StartWindow
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("GUILOAD")
	return

def MoviesPress():
	global StartWindow
	GemRB.UnloadWindow(StartWindow)
	GemRB.SetNextScript("GUIMOVIE")
	return
	
def QuitPress():
	global StartWindow, QuitWindow
	GemRB.SetVisible(StartWindow, 0)
	QuitWindow = GemRB.LoadWindow(22)
	CancelButton = GemRB.GetControl(QuitWindow, 2)
	GemRB.SetEvent(QuitWindow, CancelButton, IE_GUI_BUTTON_ON_PRESS, "QuitCancelPress")
	
	QuitButton = GemRB.GetControl(QuitWindow, 1)
	GemRB.SetEvent(QuitWindow, QuitButton, IE_GUI_BUTTON_ON_PRESS, "QuitQuitPress")
	
	TextArea = GemRB.GetControl(QuitWindow, 0)
	GemRB.SetText(QuitWindow, CancelButton, 13727)
	GemRB.SetText(QuitWindow, QuitButton, 15417)
	GemRB.SetText(QuitWindow, TextArea, 19532)
	GemRB.SetVisible(QuitWindow, 1)
	return
	
def QuitCancelPress():
	global StartWindow, QuitWindow
	GemRB.UnloadWindow(QuitWindow)
	GemRB.SetVisible(StartWindow, 1)
	return
	
def QuitQuitPress():
	GemRB.Quit()
	return

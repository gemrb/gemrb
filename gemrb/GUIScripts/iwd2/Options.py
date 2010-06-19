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

# Options.py - scripts to control options windows mostly from GUIOPT winpack

import GemRB

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	GemRB.LoadWindowPack("GUIOPT", 800, 600)

	MessageBarWindow = GemRB.LoadWindow(0)
	MessageBarWindow.SetVisible(WINDOW_VISIBLE) #This will startup the window as grayed

	CharactersBarWindow = GemRB.LoadWindow(1)
	CharactersBarWindow.SetVisible(WINDOW_VISIBLE)

	GemRB.DrawWindows()

	MessageBarWindow.SetVisible(WINDOW_INVISIBLE)
	CharactersBarWindow.SetVisible(WINDOW_INVISIBLE)

	if MessageBarWindow:
		MessageBarWindow.Unload()
	if CharactersBarWindow:
		CharactersBarWindow.Unload()

	OptionsWindow = GemRB.LoadWindow(13)
	OptionsWindow.SetFrame ()

	VersionLabel = OptionsWindow.GetControl(0x1000000B)
	VersionLabel.SetText(GEMRB_VERSION)

	GraphicsButton = OptionsWindow.GetControl(7)
	SoundButton = OptionsWindow.GetControl(8)
	GamePlayButton = OptionsWindow.GetControl(9)
	MoviesButton = OptionsWindow.GetControl(14)
	KeyboardButton = OptionsWindow.GetControl(13)
	ReturnButton = OptionsWindow.GetControl(11)

	GraphicsButton.SetText(17162)
	GraphicsButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "GraphicsPress")
	SoundButton.SetText(17164)
	SoundButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "SoundPress")
	GamePlayButton.SetText(17165)
	GamePlayButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "GamePlayPress")
	MoviesButton.SetText(15415)
	MoviesButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "MoviePress")
	KeyboardButton.SetText(33468)
	KeyboardButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "KeyboardPress")

	ReturnButton.SetText(10308)
	ReturnButton.SetEventByName(IE_GUI_BUTTON_ON_PRESS, "ReturnPress")
	ReturnButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	OptionsWindow.SetVisible(WINDOW_VISIBLE)

	return

def ReturnPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Start")
	return

def GraphicsPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Graphics")
	return

def SoundPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Sound")
	return

def GamePlayPress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("GamePlay")
	return

def MoviePress():
	global OptionsWindow
	if OptionsWindow:
		OptionsWindow.Unload()
	GemRB.SetNextScript("Movies")
	return

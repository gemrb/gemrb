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
import GUIOPT
from GUIDefines import *

def OnLoad():
	global OptionsWindow

	MessageBarWindow = GemRB.LoadWindow(0, "GUIOPT")

	CharactersBarWindow = GemRB.LoadWindow(1, "GUIOPT")

	if MessageBarWindow:
		MessageBarWindow.Close ()
	if CharactersBarWindow:
		CharactersBarWindow.Close ()

	OptionsWindow = GemRB.LoadWindow(13, "GUIOPT")

	VersionLabel = OptionsWindow.GetControl(0x1000000B)
	VersionLabel.SetText(GemRB.Version)

	GraphicsButton = OptionsWindow.GetControl(7)
	SoundButton = OptionsWindow.GetControl(8)
	GamePlayButton = OptionsWindow.GetControl(9)
	MoviesButton = OptionsWindow.GetControl(14)
	KeyboardButton = OptionsWindow.GetControl(13)
	ReturnButton = OptionsWindow.GetControl(11)

	GraphicsButton.SetText(17162)
	GraphicsButton.OnPress (GUIOPT.OpenVideoOptionsWindow)
	SoundButton.SetText(17164)
	SoundButton.OnPress (GUIOPT.OpenAudioOptionsWindow)
	GamePlayButton.SetText(17165)
	GamePlayButton.OnPress (GUIOPT.OpenGameplayOptionsWindow)
	MoviesButton.SetText(15415)
	MoviesButton.OnPress (GUIOPT.OpenMovieWindow)
	KeyboardButton.SetText(33468)
	KeyboardButton.OnPress (GUIOPT.OpenHotkeyOptionsWindow)

	ReturnButton.SetText(10308)
	def CloseStartOptions ():
		GemRB.GetView ("STARTWIN").SetDisabled (False)
		OptionsWindow.Close ()
	ReturnButton.OnPress (CloseStartOptions)
	ReturnButton.MakeEscape()

	OptionsWindow.Focus()
	GemRB.GetView ("STARTWIN").SetDisabled (True)

	return

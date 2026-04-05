# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Options.py - scripts to control options windows mostly from GUIOPT winpack

import GemRB
import GUIOPT
import GUIOPTExtra
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

	# GemRB extras
	frame = KeyboardButton.GetFrame ()
	GUIOPTExtra.AddGemRBOptionsButton (OptionsWindow, frame, 0, 60, "GBTNLRG2")

	OptionsWindow.Focus()
	GemRB.GetView ("STARTWIN").SetDisabled (True)

	return

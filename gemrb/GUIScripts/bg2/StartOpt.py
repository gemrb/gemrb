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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
import GemRB
import GUIOPT
import GUIOPTExtra
import GameCheck

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	OptionsWindow = GemRB.LoadWindow(13, "GUIOPT")
	if GameCheck.HasTOB() and GemRB.GetVar("oldgame") == 1:
		OptionsWindow.SetBackground("STARTOLD")

	y = 450
	w = 640
	if GameCheck.IsBG2EE ():
		y = GemRB.GetSystemVariable (SV_HEIGHT) - 100
		w = GemRB.GetSystemVariable (SV_WIDTH)
	elif GameCheck.IsBGEE ():
		y = GemRB.GetSystemVariable (SV_HEIGHT) - 55
		w = GemRB.GetSystemVariable (SV_WIDTH)
	Label = OptionsWindow.CreateLabel(0x0fff0000, 0, y, w, 30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText (GemRB.Version)

	SoundButton = OptionsWindow.GetControl(8)
	GameButton = OptionsWindow.GetControl(9)
	GraphicButton = OptionsWindow.GetControl(7)
	BackButton = OptionsWindow.GetControl(11)

	if GameCheck.IsAnyEE ():
		MoviesButton = OptionsWindow.GetControl (12)
		MoviesButton.SetText (15415)
		MoviesButton.OnPress (lambda: GemRB.SetNextScript ("GUIMOVIE"))

		if GameCheck.IsBG2EE ():
			OptionsWindow.DeleteControl (15)

		frame = SoundButton.GetFrame ()
		GUIOPTExtra.AddGemRBOptionsButton (OptionsWindow, frame, 0, 80, "STARTMBT", 1)
	elif GameCheck.IsBG2 ():
		frame = SoundButton.GetFrame ()
		frame["w"] -= 25
		GUIOPTExtra.AddGemRBOptionsButton (OptionsWindow, frame, 0, 50, "GMPCONNC", 2)

	SoundButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GraphicButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	BackButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SoundButton.SetText(17164)
	GameButton.SetText(17165)
	GraphicButton.SetText(17162)
	BackButton.SetText (12896)

	SoundButton.OnPress (GUIOPT.OpenAudioOptionsWindow )
	GameButton.OnPress (GUIOPT.OpenGameplayOptionsWindow)
	GraphicButton.OnPress (GUIOPT.OpenVideoOptionsWindow)
	BackButton.OnPress (OptionsWindow.Close)
	BackButton.MakeEscape()
	OptionsWindow.Focus()
	return

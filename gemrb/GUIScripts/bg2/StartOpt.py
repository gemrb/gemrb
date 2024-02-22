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
import GameCheck

OptionsWindow = 0

def OnLoad():
	global OptionsWindow
	OptionsWindow = GemRB.LoadWindow(13, "GUIOPT")
	if GameCheck.HasTOB() and GemRB.GetVar("oldgame") == 1:
		OptionsWindow.SetBackground("STARTOLD")
	Label = OptionsWindow.CreateLabel(0x0fff0000, 0,450,640,30, "REALMS", "", IE_FONT_SINGLE_LINE | IE_FONT_ALIGN_CENTER)
	Label.SetText (GemRB.Version)
	SoundButton = OptionsWindow.GetControl(8)
	GameButton = OptionsWindow.GetControl(9)
	GraphicButton = OptionsWindow.GetControl(7)
	BackButton = OptionsWindow.GetControl(11)
	SoundButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GameButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	GraphicButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	BackButton.SetStatus(IE_GUI_BUTTON_ENABLED)
	SoundButton.SetText(17164)
	GameButton.SetText(17165)
	GraphicButton.SetText(17162)
	BackButton.SetText(10308)
	SoundButton.OnPress (GUIOPT.OpenAudioOptionsWindow )
	GameButton.OnPress (GUIOPT.OpenGameplayOptionsWindow)
	GraphicButton.OnPress (GUIOPT.OpenVideoOptionsWindow)
	BackButton.OnPress (OptionsWindow.Close)
	BackButton.MakeEscape()
	OptionsWindow.Focus()
	return

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
#sound options
import GemRB

SoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global SoundWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	SoundWindow = GemRB.LoadWindowObject(7)
	TextAreaControl = SoundWindow.GetControl(14)
	AmbientButton = SoundWindow.GetControl(16)
	AmbientSlider = SoundWindow.GetControl(1)
	SoundEffectsButton = SoundWindow.GetControl(17)
	SoundEffectsSlider = SoundWindow.GetControl(2)
	DialogueButton = SoundWindow.GetControl(18)
	DialogueSlider = SoundWindow.GetControl(3)
	MusicButton = SoundWindow.GetControl(19)
	MusicSlider = SoundWindow.GetControl(4)
	MoviesButton = SoundWindow.GetControl(20)
	MoviesSlider = SoundWindow.GetControl(22)
	EnvironmentalButton = SoundWindow.GetControl(28)
	EnvironmentalButtonB = SoundWindow.GetControl(26)
	CharacterSoundButton = SoundWindow.GetControl(13)
	OkButton = SoundWindow.GetControl(24)
	CancelButton = SoundWindow.GetControl(25)
	TextAreaControl.SetText(18040)
	CharacterSoundButton.SetText(17778)
	OkButton.SetText(11973)
	CancelButton.SetText(13727)
	AmbientButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AmbientPress")
	AmbientSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "AmbientPress")
	AmbientSlider.SetVarAssoc("Volume Ambients",10)

	SoundEffectsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SoundEffectsPress")
	SoundEffectsSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "SoundEffectsPress")
	SoundEffectsSlider.SetVarAssoc("Volume SFX",10)
	
	DialogueButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "DialoguePress")
	DialogueSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "DialoguePress")
	DialogueSlider.SetVarAssoc("Volume Voices",10)

	MusicButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MusicPress")
	MusicSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "MusicPress")
	MusicSlider.SetVarAssoc("Volume Music",10)

	MoviesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MoviesPress")
	MoviesSlider.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "MoviesPress")
	MoviesSlider.SetVarAssoc("Volume Movie",10)

	EnvironmentalButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPress")
	EnvironmentalButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EnvironmentalPress")
	EnvironmentalButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX,OP_OR)
	EnvironmentalButtonB.SetVarAssoc("Environmental Audio",1)

	CharacterSoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CharacterSoundPress")
	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	SoundWindow.SetVisible(WINDOW_VISIBLE)
	return
	
def AmbientPress():
	TextAreaControl.SetText(18008)
	GemRB.UpdateAmbientsVolume ()
	return
	
def SoundEffectsPress():
	TextAreaControl.SetText(18009)
	return
	
def DialoguePress():
	TextAreaControl.SetText(18010)
	return
	
def MusicPress():
	TextAreaControl.SetText(18011)
	GemRB.UpdateMusicVolume ()
	return
	
def MoviesPress():
	TextAreaControl.SetText(18012)
	return
	
def EnvironmentalPress():
	TextAreaControl.SetText(18022)
	return
	
def CharacterSoundPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("GUIOPT12")
	return
	
def OkPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("StartOpt")
	return
	
def CancelPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("StartOpt")
	return

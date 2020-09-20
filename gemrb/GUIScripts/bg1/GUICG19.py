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
#character generation, sounds (GUICG19)
import GemRB
from GUIDefines import *
from ie_restype import RES_WAV
from ie_sounds import CHAN_CHAR1
import CharGenCommon
import GUICommon

SoundSequence = [ "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m" ]
VoiceList = 0
CharSoundWindow = 0
SoundIndex = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	CharSoundWindow=GemRB.LoadWindow(19, "GUICG")

	VoiceList = CharSoundWindow.GetControl (45)
	if GemRB.GetVar ("Gender")==1:
		VoiceList.SetVarAssoc ("Selected", 3) #first male sound
	else:
		VoiceList.SetVarAssoc ("Selected", 0)
	VoiceList.ListResources (CHR_SOUNDS)

	PlayButton = CharSoundWindow.GetControl (47)
	PlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PlayPress)
	PlayButton.SetText (17318)

	TextArea = CharSoundWindow.GetControl (50)
	TextArea.SetText (11315)

	BackButton = CharSoundWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = CharSoundWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	VoiceList.SetEvent(IE_GUI_TEXTAREA_ON_SELECT, ChangeVoice)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, lambda: CharGenCommon.back(CharSoundWindow))
	CharSoundWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def PlayPress():
	global CharSoundWindow, SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText()
	# SClassID.h -> IE_WAV_CLASS_ID = 0x00000004
	while (not GemRB.HasResource (CharSound + SoundSequence[SoundIndex], RES_WAV)):
		NextSound()
	# play the sound like it was a speech, so any previous yells are quieted
	GemRB.PlaySound (CharSound + SoundSequence[SoundIndex], CHAN_CHAR1, 0, 0, 4)
	NextSound()
	return

def NextSound():
	global SoundIndex, SoundSequence
	SoundIndex += 1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0
	return

def ChangeVoice():
	global SoundIndex
	SoundIndex = 0
	return

def NextPress():
	CharSoundWindow.Close()
	CharSound = VoiceList.QueryText ()
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerSound(MyChar,CharSound)
	CharGenCommon.next()
	return

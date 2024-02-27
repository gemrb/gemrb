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

import CharGenCommon
import GUICommon
from ie_restype import *
from ie_sounds import CHAN_CHAR1, SND_SPEECH
from ie_stats import IE_SEX

VoiceList = 0
CharSoundWindow = 0

# the available sounds
SoundSequence = [ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', \
				'm', 's', 't', 'u', 'v', '_', 'x', 'y', 'z', '0', '1', '2', \
				'3', '4', '5', '6', '7', '8', '9']
SoundIndex = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	CharSoundWindow=GemRB.LoadWindow(19, "GUICG")
	CharGenCommon.PositionCharGenWin(CharSoundWindow)

	VoiceList = CharSoundWindow.GetControl (45)
	Voices = VoiceList.ListResources (CHR_SOUNDS)
	GUICommon.AddDefaultVoiceSet (VoiceList, Voices)
	VoiceList.OnSelect (ChangeVoice)
	# preselect the default entry to avoid an infinite loop if Play is pressed immediately
	VoiceList.SetValue (0)

	PlayButton = CharSoundWindow.GetControl (47)
	PlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	PlayButton.OnPress (PlayPress)
	PlayButton.SetText (17318)

	TextArea = CharSoundWindow.GetControl (50)
	TextArea.SetText (11315)

	BackButton = CharSoundWindow.GetControl(10)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = CharSoundWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	CharSoundWindow.Focus()
	return

def PlayPress():
	global CharSoundWindow, SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText()
	MyChar = GemRB.GetVar ("Slot")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)
	CharSound = GUICommon.OverrideDefaultVoiceSet (Gender, CharSound)

	while (not GemRB.HasResource (CharSound + SoundSequence[SoundIndex], RES_WAV)):
		NextSound()
	# play the sound like it was a speech, so any previous yells are quieted
	GemRB.PlaySound (CharSound + SoundSequence[SoundIndex], CHAN_CHAR1, 0, 0, SND_SPEECH)
	NextSound()
	return

def NextSound():
	global SoundIndex, SoundSequence
	SoundIndex += 1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0
	return

# When a new voice is selected, play the sounds again from the beginning of the sequence
def ChangeVoice():
	global SoundIndex
	SoundIndex = 0
	return

def BackPress():
	global CharSoundWindow

	if CharSoundWindow:
		CharSoundWindow.Close ()
	GemRB.SetNextScript("GUICG13") 
	return

def NextPress():
	global CharSoundWindow

	CharSound = VoiceList.QueryText ()
	MyChar = GemRB.GetVar ("Slot")
	Gender = GemRB.GetPlayerStat (MyChar, IE_SEX)
	CharSound = GUICommon.OverrideDefaultVoiceSet (Gender, CharSound)
	GemRB.SetPlayerSound (MyChar, CharSound)

	if CharSoundWindow:
		CharSoundWindow.Close ()
	GemRB.SetNextScript("CharGen8") #name
	return

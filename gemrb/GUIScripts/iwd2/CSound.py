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
#character generation, sound (GUICG19)
import GemRB

import CharOverview
from GUIDefines import *

SoundWindow = 0
TextAreaControl = 0
DoneButton = 0
SoundIndex = 0
VerbalConstants = None

def OnLoad():
	global SoundWindow, TextAreaControl, DoneButton, VerbalConstants

	#this hack will redraw the base CG window
	SoundWindow = GemRB.LoadWindow(19, "GUICG")
	CharOverview.PositionCharGenWin (SoundWindow)

	CharSoundTable = GemRB.LoadTable ("CHARSND")
	VerbalConstants =  [CharSoundTable.GetRowName(i) for i in range(CharSoundTable.GetRowCount())]

	BackButton = SoundWindow.GetControl(10)
	BackButton.SetText(15416)
	BackButton.MakeEscape()

	DoneButton = SoundWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.MakeDefault()

	TextAreaControl = SoundWindow.GetControl(50)
	TextAreaControl.SetText(17236)

	TextAreaControl = SoundWindow.GetControl(45)
	TextAreaControl.OnSelect (SelectSound)

	TextAreaControl.ListResources (CHR_SOUNDS)
	if GemRB.GetVar ("Gender") == 2:
		GemRB.SetVar ("Sound", 0) #first female sound
	else:
		GemRB.SetVar ("Sound", 21)
	TextAreaControl.SetVarAssoc("Sound", 0)

	PlayButton = SoundWindow.GetControl(47)
	PlayButton.SetText(17318)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	PlayButton.OnPress (PlayPress)
	SoundWindow.Focus()
	return

def BackPress():
	if SoundWindow:
		SoundWindow.Close ()
	GemRB.SetNextScript("Appearance")
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	return

def NextPress():
	if SoundWindow:
		SoundWindow.Close ()
	GemRB.SetNextScript("CharGen8") #name
	return

def SelectSound():
	PlayPress ()
	return

def PlayPress():
	global SoundIndex

	CharSound = TextAreaControl.QueryText()
	MyChar = GemRB.GetVar ("Slot")

	GemRB.SetPlayerSound (MyChar, CharSound)
	# play sound as sound slot
	GemRB.VerbalConstant (MyChar, int(VerbalConstants[SoundIndex]))

	NextSound()
	return

def NextSound():
	global SoundIndex
	SoundIndex += 1
	if SoundIndex >= len(VerbalConstants):
		SoundIndex = 0
	return

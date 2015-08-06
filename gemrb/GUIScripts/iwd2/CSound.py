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
from GUIDefines import *

SoundWindow = 0
TextAreaControl = 0
DoneButton = 0
SoundIndex = 0
VerbalConstants = None

def OnLoad():
	global SoundWindow, TextAreaControl, DoneButton, VerbalConstants

	GemRB.LoadWindowPack("GUICG", 800,  600)
	#this hack will redraw the base CG window
	SoundWindow = GemRB.LoadWindow(19)
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	CharSoundTable = GemRB.LoadTable ("CHARSND")
	VerbalConstants =  [CharSoundTable.GetRowName(i) for i in range(CharSoundTable.GetRowCount())]

	BackButton = SoundWindow.GetControl(10)
	BackButton.SetText(15416)
	BackButton.SetFlags(IE_GUI_BUTTON_CANCEL,OP_OR)

	DoneButton = SoundWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = SoundWindow.GetControl(50)
	TextAreaControl.SetText(17236)

	TextAreaControl = SoundWindow.GetControl(45)
	TextAreaControl.SetVarAssoc("Sound", 0)
	TextAreaControl.SetEvent(IE_GUI_TEXTAREA_ON_SELECT, SelectSound)
	RowCount=TextAreaControl.ListResources(CHR_SOUNDS)

	PlayButton = SoundWindow.GetControl(47)
	PlayButton.SetText(17318)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, PlayPress)
	SoundWindow.SetVisible(WINDOW_VISIBLE)
	return

def BackPress():
	if SoundWindow:
		SoundWindow.Unload()
	GemRB.SetNextScript("Appearance")
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	return

def NextPress():
	GemRB.SetToken("VoiceSet", TextAreaControl.QueryText())
	if SoundWindow:
		SoundWindow.Unload()
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

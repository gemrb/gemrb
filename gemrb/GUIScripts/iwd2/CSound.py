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
TopIndex = 0

def OnLoad():
	global SoundWindow, TextAreaControl, DoneButton, TopIndex

	GemRB.LoadWindowPack("GUICG", 800,  600)
	#this hack will redraw the base CG window
	SoundWindow = GemRB.LoadWindow(19)
	GemRB.SetVar("Sound",0)  #scrapping the sound value

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
	RowCount=TextAreaControl.ListResources(CHR_SOUNDS)

	DefaultButton = SoundWindow.GetControl(47)
	DefaultButton.SetText(33479)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DefaultButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, DefaultPress)
	SoundWindow.SetVisible(WINDOW_VISIBLE)
	return

def DefaultPress():
	GemRB.SetVar("Sound",0)  #scrapping the sound value
	TextAreaControl.SetVarAssoc("Sound", 0)
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

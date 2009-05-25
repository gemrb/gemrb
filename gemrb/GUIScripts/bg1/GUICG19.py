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
# $Id$
#
#character generation, sounds (GUICG19)
import GemRB

VoiceList = 0
CharSoundWindow = 0

def OnLoad():
	global CharSoundWindow, VoiceList
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	CharSoundWindow=GemRB.LoadWindowObject(19)

	VoiceList = CharSoundWindow.GetControl (45)
	VoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	if GemRB.GetVar ("Gender")==1:
		GemRB.SetVar ("Selected", 3) #first male sound
	else:
		GemRB.SetVar ("Selected", 0)

	VoiceList.SetVarAssoc ("Selected", 0)
	RowCount=VoiceList.GetCharSounds()

	PlayButton = CharSoundWindow.GetControl (47)
	PlayButton.SetState (IE_GUI_BUTTON_ENABLED)
	PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, "PlayPress")
	PlayButton.SetText (17318)

	TextArea = CharSoundWindow.GetControl (50)
	TextArea.SetText (11315)

	BackButton = CharSoundWindow.GetControl(10)
	BackButton.SetText(15416)
	DoneButton = CharSoundWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	CharSoundWindow.SetVisible(1)
	return

def PlayPress():
	global CharSoundWindow

	CharSound = VoiceList.QueryText()
	GemRB.PlaySound (CharSound+"a")
	return

def BackPress():
	global CharSoundWindow

	if CharSoundWindow:
		CharSoundWindow.Unload()
	GemRB.SetNextScript("GUICG13") 
	return

def NextPress():
	global CharSoundWindow

	CharSound = VoiceList.QueryText ()
	GemRB.SetToken ("CharSound", CharSound)
	if CharSoundWindow:
		CharSoundWindow.Unload()
	GemRB.SetNextScript("CharGen8") #name
	return

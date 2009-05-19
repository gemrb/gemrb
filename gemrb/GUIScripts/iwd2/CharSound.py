# -*-python-*-
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id$

#Character Sound Options Menu using GUIOPT

import GemRB

CSoundWindow = 0
TextAreaControl = 0

def OnLoad():
	global CSoundWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT", 800, 600)
	CSoundWindow = GemRB.LoadWindowObject(12)
	CSoundWindow.SetFrame( )
	TextAreaControl = CSoundWindow.GetControl(16)
	SubtitlesButton = CSoundWindow.GetControl(20)
	AttackSoundButton = CSoundWindow.GetControl(18)
	MovementSoundButton = CSoundWindow.GetControl(19)
	CommandSoundButton = CSoundWindow.GetControl(21)
	SelectionSoundButton = CSoundWindow.GetControl(57)
	SubtitlesButtonB = CSoundWindow.GetControl(5)
	AttackSoundButtonB = CSoundWindow.GetControl(6)
	MovementSoundButtonB = CSoundWindow.GetControl(7)
	CSAlwaysButtonB = CSoundWindow.GetControl(8)
	CSSeldomButtonB = CSoundWindow.GetControl(9)
	CSNeverButtonB = CSoundWindow.GetControl(10)
	SSAlwaysButtonB = CSoundWindow.GetControl(58)
	SSSeldomButtonB = CSoundWindow.GetControl(59)
	SSNeverButtonB = CSoundWindow.GetControl(60)
	OkButton = CSoundWindow.GetControl(24)
	CancelButton = CSoundWindow.GetControl(25)
	TextAreaControl.SetText(18041)
	OkButton.SetText(11973)
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton.SetText(13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	SubtitlesButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SubtitlesPress")
	SubtitlesButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SubtitlesPress")
	SubtitlesButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	SubtitlesButtonB.SetVarAssoc("Subtitles", 1)
	SubtitlesButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	AttackSoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AttackSoundPress")
	AttackSoundButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AttackSoundPress")
	AttackSoundButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	AttackSoundButtonB.SetVarAssoc("Attack Sound", 1) #can't find the right variable name, this is a dummy name
	AttackSoundButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	MovementSoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MovementSoundPress")
	MovementSoundButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MovementSoundPress")
	MovementSoundButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	MovementSoundButtonB.SetVarAssoc("Movement Sound", 1) #can't find the right variable name, this is a dummy name
	MovementSoundButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	CommandSoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	CSAlwaysButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	CSAlwaysButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	CSAlwaysButtonB.SetVarAssoc("Command Sounds Frequency", 3)
	CSAlwaysButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	CSSeldomButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	CSSeldomButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	CSSeldomButtonB.SetVarAssoc("Command Sounds Frequency", 2)
	CSSeldomButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	CSNeverButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CommandSoundPress")
	CSNeverButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	CSNeverButtonB.SetVarAssoc("Command Sounds Frequency", 1)
	CSNeverButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	SelectionSoundButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	SSAlwaysButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	SSAlwaysButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	SSAlwaysButtonB.SetVarAssoc("Selection Sounds Frequency", 3)
	SSAlwaysButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	SSSeldomButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	SSSeldomButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	SSSeldomButtonB.SetVarAssoc("Selection Sounds Frequency", 2)
	SSSeldomButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	SSNeverButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionSoundPress")
	SSNeverButtonB.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	SSNeverButtonB.SetVarAssoc("Selection Sounds Frequency", 1)
	SSNeverButtonB.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	CSoundWindow.SetVisible(1)
	return

def SubtitlesPress():
	TextAreaControl.SetText(18015)
	return

def AttackSoundPress():
	TextAreaControl.SetText(18013)
	return

def MovementSoundPress():
	TextAreaControl.SetText(18014)
	return

def CommandSoundPress():
	TextAreaControl.SetText(18016)
	return

def SelectionSoundPress():
	TextAreaControl.SetText(11352)
	return

def OkPress():
	if CSoundWindow:
		CSoundWindow.Unload()
	GemRB.SetNextScript("Options")
	return

def CancelPress():
	if CSoundWindow:
		CSoundWindow.Unload()
	GemRB.SetNextScript("Options")
	return

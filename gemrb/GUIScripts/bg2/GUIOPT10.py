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
#autopause
import GemRB

def OnLoad():
	global AutoPauseWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	
	AutoPauseWindow = GemRB.LoadWindowObject(10)
	TextAreaControl = AutoPauseWindow.GetControl(15)

	ChHitButton = AutoPauseWindow.GetControl(17)
	ChHitButtonB = AutoPauseWindow.GetControl(1)

	ChInjured = AutoPauseWindow.GetControl(18)
	ChInjuredB = AutoPauseWindow.GetControl(2)

	ChDeath = AutoPauseWindow.GetControl(19)
	ChDeathB = AutoPauseWindow.GetControl(3)

	ChAttacked = AutoPauseWindow.GetControl(20)
	ChAttackedB = AutoPauseWindow.GetControl(4)

	WeaponUnusable = AutoPauseWindow.GetControl(21)
	WeaponUnusableB = AutoPauseWindow.GetControl(5)

	TargetDestroyed = AutoPauseWindow.GetControl(22)
	TargetDestroyedB = AutoPauseWindow.GetControl(13)

	EndOfRound = AutoPauseWindow.GetControl(24)
	EndOfRoundB = AutoPauseWindow.GetControl(25)

	EnemySighted = AutoPauseWindow.GetControl(27)
	EnemySightedB = AutoPauseWindow.GetControl(26)

	SpellCast = AutoPauseWindow.GetControl(30)
	SpellCastB = AutoPauseWindow.GetControl(34)

	TrapFound = AutoPauseWindow.GetControl(33)
	TrapFoundB = AutoPauseWindow.GetControl(31)

	AutopauseCenter = AutoPauseWindow.GetControl(36)
	AutopauseCenterB = AutoPauseWindow.GetControl(37)

	OkButton = AutoPauseWindow.GetControl(11)
	CancelButton = AutoPauseWindow.GetControl(14)
	TextAreaControl.SetText(18044)
	OkButton.SetText(11973)
	CancelButton.SetText(13727)

	ChHitButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChHitButtonPress")
	ChHitButtonB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	ChHitButtonB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChHitButtonPress")

	ChInjured.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChInjuredPress")
	ChInjuredB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	ChInjuredB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChInjuredPress")

	ChDeath.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChDeathPress")
	ChDeathB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	ChDeathB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChDeathPress")

	ChAttacked.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChAttackedPress")
	ChAttackedB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	ChAttackedB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ChAttackedPress")

	WeaponUnusable.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeaponUnusablePress")
	WeaponUnusableB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	WeaponUnusableB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "WeaponUnusablePress")

	TargetDestroyed.SetEvent(IE_GUI_BUTTON_ON_PRESS, "TargetDestroyedPress")
	TargetDestroyedB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	TargetDestroyedB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "TargetDestroyedPress")

	EndOfRound.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EndOfRoundPress")
	EndOfRoundB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	EndOfRoundB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EndOfRoundPress")

	EnemySighted.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EnemySightedPress")
	EnemySightedB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	EnemySightedB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "EnemySightedPress")

	SpellCast.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SpellCastPress")
	SpellCastB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	SpellCastB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SpellCastPress")

	TrapFound.SetEvent(IE_GUI_BUTTON_ON_PRESS, "TrapFoundPress")
	TrapFoundB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	TrapFoundB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "TrapFoundPress")

	AutopauseCenter.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AutopauseCenterPress")
	AutopauseCenterB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	AutopauseCenterB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AutopauseCenterPress")

	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AutopauseCenterB.SetVarAssoc("Auto Pause Center",1)

	ChHitButtonB.SetVarAssoc("Auto Pause State",1)
	ChInjuredB.SetVarAssoc("Auto Pause State",2)
	ChDeathB.SetVarAssoc("Auto Pause State",4)
	ChAttackedB.SetVarAssoc("Auto Pause State",8)
	WeaponUnusableB.SetVarAssoc("Auto Pause State",16)
	TargetDestroyedB.SetVarAssoc("Auto Pause State",32)
	EndOfRoundB.SetVarAssoc("Auto Pause State",64)
	EnemySightedB.SetVarAssoc("Auto Pause State",128)
	SpellCastB.SetVarAssoc("Auto Pause State",256)
	TrapFoundB.SetVarAssoc("Auto Pause State",512)

	AutoPauseWindow.SetVisible(1)
	return

def ChHitButtonPress():
	TextAreaControl.SetText(18032)
	return

def ChInjuredPress():
	TextAreaControl.SetText(18033)
	return

def ChDeathPress():
	TextAreaControl.SetText(18034)
	return

def ChAttackedPress():
	TextAreaControl.SetText(18035)
	return

def WeaponUnusablePress():
	TextAreaControl.SetText(18036)
	return

def TargetDestroyedPress():
	TextAreaControl.SetText(18037)
	return

def EndOfRoundPress():
	TextAreaControl.SetText(10640)
	return

def EnemySightedPress():
	TextAreaControl.SetText(23514)
	return

def SpellCastPress():
	TextAreaControl.SetText(58171)
	return

def TrapFoundPress():
	TextAreaControl.SetText(31872)
	return

def AutopauseCenterPress():
	TextAreaControl.SetText(10571)
	return

def OkPress():
	if AutoPauseWindow:
		AutoPauseWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return

def CancelPress():
	if AutoPauseWindow:
		AutoPauseWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return

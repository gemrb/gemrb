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
#character generation, name (GUICG5)
import GemRB
from GUIDefines import *

import CharGenCommon
import GUICommon


NameWindow = 0
NameField = 0
DoneButton = 0

def OnLoad():
	global NameWindow, NameField, DoneButton
	
	NameWindow = GemRB.LoadWindow(5, "GUICG")

	BackButton = NameWindow.GetControl(3)
	BackButton.SetText(15416)

	DoneButton = NameWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	NameField = NameWindow.GetControl(2)
	NameField.SetText (GemRB.GetToken ("CHARNAME"))
	EditChange ()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (lambda: CharGenCommon.back(NameWindow))
	NameField.OnChange (EditChange)
	NameWindow.ShowModal(MODAL_SHADOW_GRAY)
	NameField.Focus()
	return

def NextPress():
	NameWindow.Close()
	Name = NameField.QueryText()
	#check length?
	#seems like a good idea to store it here for the time being
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerName (MyChar, Name, 0)
	GemRB.SetToken ("CHARNAME", Name)
	CharGenCommon.next()
	return

def EditChange():
	Name = NameField.QueryText()
	if len(Name)==0:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

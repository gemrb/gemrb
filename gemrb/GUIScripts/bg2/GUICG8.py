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
#character generation, race (GUICG2)
import GemRB
from GUICommonWindows import RaceTable

RaceWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global RaceWindow, TextAreaControl, DoneButton
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	RaceWindow = GemRB.LoadWindowObject(8)

	RaceCount = RaceTable.GetRowCount()

	for i in range(2,RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
	for i in range(2, RaceCount+2):
		Button = RaceWindow.GetControl(i)
		Button.SetText(RaceTable.GetValue(i-2,0) )
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,"RacePress")
		Button.SetVarAssoc("Race",i-1)

	BackButton = RaceWindow.GetControl(i+2)  #i=8 now (when race count is 7)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = RaceWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)

	TextAreaControl = RaceWindow.GetControl(12)
	TextAreaControl.SetText(17237)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	RaceWindow.SetVisible(1)
	return

def RacePress():
	Race = GemRB.GetVar("Race")-1
	TextAreaControl.SetText(RaceTable.GetValue(Race,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen2")
	GemRB.SetVar("Race",0)  #scrapping the race value
	return

def NextPress():
	if RaceWindow:
		RaceWindow.Unload()
	GemRB.SetNextScript("CharGen3") #class
	return

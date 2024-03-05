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
#character generation, multi-class (GUICG10)
import GemRB

import GUICommon
import CharGenCommon
import CommonTables

from GUIDefines import *
from ie_stats import *

TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global TextAreaControl, DoneButton

	ClassWindow = GemRB.LoadWindow(10, "GUICG")

	ClassCount = CommonTables.Classes.GetRowCount()+1
	RaceName = CommonTables.Races.GetRowName(GemRB.GetVar("Race")-1 )

	j=0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i-1)
		if CommonTables.Classes.GetValue (ClassName, "MULTI") == 0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		j = j + 1

	j=0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, RaceName)
		if CommonTables.Classes.GetValue (ClassName, "MULTI") == 0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)

		t = CommonTables.Classes.GetValue (ClassName, "NAME_REF")
		Button.SetText(t )
		j=j+1
		if Allowed ==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.OnPress (ClassPress)
		Button.SetVarAssoc("Class", i) #multiclass, actually

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.OnPress (lambda: NextPress(ClassWindow))
	BackButton.OnPress (lambda: CharGenCommon.back(ClassWindow))
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	ClassWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def ClassPress():
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress(Window):
	Window.Close()
	#class	
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	CharGenCommon.next()

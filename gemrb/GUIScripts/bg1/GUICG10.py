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
from GUICommon import CloseOtherWindow, RaceTable, ClassTable

from CharGenCommon import * 

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton

	GemRB.LoadWindowPack("GUICG")
	ClassWindow = GemRB.LoadWindowObject(10)

	CloseOtherWindow (ClassWindow.Unload)

	ClassCount = ClassTable.GetRowCount()+1
	RaceName = RaceTable.GetRowName(GemRB.GetVar("Race")-1 )

	j=0
	for i in range(1,ClassCount):
		if ClassTable.GetValue(i-1,4)==0:
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
		ClassName = ClassTable.GetRowName(i-1)
		Allowed = ClassTable.GetValue(ClassName, RaceName)
		if ClassTable.GetValue(i-1,4)==0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)

		t = ClassTable.GetValue(i-1, 0)
		Button.SetText(t )
		j=j+1
		if Allowed ==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS,  "ClassPress")
		Button.SetVarAssoc("Class", i) #multiclass, actually

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	ClassWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def ClassPress():
	Class = GemRB.GetVar("Class")-1
	TextAreaControl.SetText(ClassTable.GetValue(Class,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	#class	
	ClassIndex = GemRB.GetVar ("Class")-1
	Class = ClassTable.GetValue (ClassIndex, 5)
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	next()

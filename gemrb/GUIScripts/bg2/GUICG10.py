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
import CommonTables
from ie_stats import *
from GUIDefines import *
import GUICG2

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0
ButtonCount = 0
MCRowIndices = []

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, MyChar, ButtonCount, MCRowIndices
	
	GemRB.LoadWindowPack("GUICG", 640, 480)
	ClassWindow = GemRB.LoadWindow(10)

	MyChar = GemRB.GetVar ("Slot")
	ClassCount = CommonTables.Classes.GetRowCount()+1
	Race = GemRB.GetPlayerStat (MyChar, IE_RACE)
	RaceName = CommonTables.Races.GetRowName(CommonTables.Races.FindValue (3, Race) )

	MCRowIndices = []
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, RaceName)
		if CommonTables.Classes.GetValue (ClassName, "MULTI") == 0:
			# not a multiclass
			continue
		MCRowIndices.append((i-1, Allowed))

	ButtonCount = 10
	if len(MCRowIndices) > 10:
		# add another button, there's another slot left
		ClassWindow.CreateButton (15, 18, 250, 271, 20)
		extramc = ClassWindow.GetControl(15)
		extramc.SetState(IE_GUI_BUTTON_DISABLED)
		extramc.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		extramc.SetSprites("GUICGBC",0, 0,1,2,3)
		ButtonCount = 11
	if len(MCRowIndices) > 11:
		# bah, also add a scrollbar
		ClassWindow.CreateScrollBar(1000, 290, 50, 16, 220, "GUISCRCW")
		ScrollBar = ClassWindow.GetControl (1000)
		ScrollBar.SetVarAssoc("TopIndex", len(MCRowIndices)-10)
		ScrollBar.SetEvent(IE_GUI_SCROLLBAR_ON_CHANGE, RedrawMCs)
		ScrollBar.SetDefaultScrollBar()

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	RedrawMCs()
	ClassWindow.SetVisible(WINDOW_VISIBLE)
	return

def ClassPress():
	GUICG2.SetClass()
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	GemRB.SetVar("Class",0)  # scrapping it
	if ClassWindow:
		ClassWindow.Unload()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	GUICG2.SetClass()
	if ClassWindow:
		ClassWindow.Unload()

	# find the class from the class table
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	GemRB.SetNextScript("CharGen4") #alignment
	return

#TODO: deal with the potential scrollbar
def RedrawMCs():
	for i in range (2, 2+ButtonCount): # loop over the available buttons
		if i == 12:
			Button = ClassWindow.GetControl(15) # the extra button
		else:
			Button = ClassWindow.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

		t = GUICommon.GetClassRowName (MCRowIndices[i-2][0], "index")
		t = CommonTables.Classes.GetValue(t, "NAME_REF")
		Button.SetText(t )
		if not MCRowIndices[i-2][1]:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, ClassPress)
		Button.SetVarAssoc("Class", MCRowIndices[i-2][0]+1) #multiclass, actually; just a wierd system

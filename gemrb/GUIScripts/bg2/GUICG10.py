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
import CharGenCommon
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
	
	ClassWindow = GemRB.LoadWindow(10, "GUICG")
	CharGenCommon.PositionCharGenWin(ClassWindow)

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
		extramc = ClassWindow.CreateButton (15, 18, 250, 271, 20)
		extramc.SetState(IE_GUI_BUTTON_DISABLED)
		extramc.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		extramc.SetSprites("GUICGBC",0, 0,1,2,3)
		ButtonCount = 11
	if len(MCRowIndices) > 11:
		# bah, also add a scrollbar
		ScrollBar = ClassWindow.CreateScrollBar(1000, {'x' : 290, 'y' : 50, 'w' : 16, 'h' : 220}, "GUISCRCW")
		ScrollBar.SetVarAssoc("TopIndex", 0, 0, len(MCRowIndices) - 11)
		ScrollBar.OnChange (RedrawMCs)
		ClassWindow.SetEventProxy(ScrollBar)

	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = ClassWindow.GetControl(12)
	TextAreaControl.SetText(17244)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	DoneButton.SetDisabled(True)
	RedrawMCs()
	ClassWindow.Focus()
	return

def ClassPress():
	GUICG2.SetClass()
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
	DoneButton.SetDisabled(False)
	return

def BackPress():
	GemRB.SetVar("Class",0)  # scrapping it
	if ClassWindow:
		ClassWindow.Close ()
	GemRB.SetNextScript("GUICG2")
	return

def NextPress():
	GUICG2.SetClass()
	if ClassWindow:
		ClassWindow.Close ()

	# find the class from the class table
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	GemRB.SetNextScript("CharGen4") #alignment
	return

def RedrawMCs():
	for i in range (2, 2+ButtonCount): # loop over the available buttons
		if i == 12:
			Button = ClassWindow.GetControl(15) # the extra button
		else:
			Button = ClassWindow.GetControl(i)
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)

		offset = GemRB.GetVar ("TopIndex") or 0
		t = GUICommon.GetClassRowName (MCRowIndices[i - 2 + offset][0], "index")
		t = CommonTables.Classes.GetValue(t, "NAME_REF")
		Button.SetText(t )
		if not MCRowIndices[i - 2 + offset][1]:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.OnPress (ClassPress)
		Button.SetVarAssoc("Class", MCRowIndices[i - 2 + offset][0] + 1) # multiclass, actually; just a weird system

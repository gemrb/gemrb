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
#character generation, class (GUICG2)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import CommonTables


ClassWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton

	MyChar = GemRB.GetVar ("Slot")
	
	ClassCount = CommonTables.Classes.GetRowCount()+1
	ClassWindow = GemRB.LoadWindow(2, "GUICG")
	RaceRow = CommonTables.Races.FindValue(3,GemRB.GetPlayerStat (MyChar, IE_RACE))
	RaceName = CommonTables.Races.GetRowName(RaceRow)

	GemRB.SetVar("Class", 0)
	GemRB.SetVar("Multi Class", 0)
	GemRB.SetVar("Specialist", 0)
	GemRB.SetVar("Class Kit", 0)
	GemRB.SetVar("MAGESCHOOL", 0)

	HasIllusion = 0
	HasMulti = 0

	for i in range(1, ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, RaceName)

		# If we ever come across an Allowed-value of 2, then we know the
		# character is limited to the illusionist mageschool.
		if Allowed == 2:
			HasIllusion = 1

		# Multiclasses don't have their own buttons in this window, so
		# don't setup buttons for them here. For later evaluation we have
		# to remember if any multiclass is allowed for this race.
		if CommonTables.Classes.GetValue(ClassName, "MULTI"):
			if Allowed != 0:
				HasMulti = 1
			continue

		# Only enable the button if this class is allowed by this race.
		if Allowed == 1:
			btnState = IE_GUI_BUTTON_ENABLED
		else:
			btnState = IE_GUI_BUTTON_DISABLED

		Button = ClassWindow.GetControl(i+1)
		Button.SetVarAssoc("Class", i)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
		Button.SetState(btnState) # reset from SELECTED after SetVarAssoc
		Button.SetText(CommonTables.Classes.GetValue(ClassName, "NAME_REF"))
		Button.OnPress(ClassPress)

	# restore after SetVarAssoc
	GemRB.SetVar("Class", 0)

	# propagate limitation to illusionist mageschool
	if HasIllusion == 1:
		GemRB.SetVar("MAGESCHOOL", 5)

	MultiClassButton = ClassWindow.GetControl(10)
	MultiClassButton.SetText(11993)
	if HasMulti == 0:
		MultiClassButton.SetState(IE_GUI_BUTTON_DISABLED)

	Allowed = CommonTables.Classes.GetValue ("MAGE", RaceName)
	SpecialistButton = ClassWindow.GetControl(11)
	SpecialistButton.SetText(11994)
	if Allowed == 0:
		SpecialistButton.SetState(IE_GUI_BUTTON_DISABLED)
	
	BackButton = ClassWindow.GetControl(14)
	BackButton.SetText(15416)
	DoneButton = ClassWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = ClassWindow.GetControl(13)

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	if ClassName == "":
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF"))
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)

	MultiClassButton.OnPress (MultiClassPress)
	SpecialistButton.OnPress (SpecialistPress)
	DoneButton.OnPress (NextPress)
	BackButton.OnPress (lambda: CharGenCommon.back(ClassWindow))
	ClassWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def MultiClassPress():
	ClassWindow.Close ()
	GemRB.SetVar("Multi Class",1)
	CharGenCommon.next()

def SpecialistPress():
	ClassWindow.Close ()
	GemRB.SetVar("Specialist",1)

	GemRB.SetVar("Class Kit", 0)
	GemRB.SetVar("Class", 6)
	CharGenCommon.next()
	
def ClassPress():
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.Classes.GetValue (ClassName, "DESC_REF") )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress():
	ClassWindow.Close()
	# find the class from the class table
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.Classes.GetValue (ClassName, "ID")
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)
	CharGenCommon.next()
	

# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

	ClassRaceTable = GemRB.LoadTable ("clsrcreq")
	j=0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName (i-1)
		Allowed = ClassRaceTable.GetValue (ClassName, RaceName, GTV_INT)
		if CommonTables.Classes.GetValue (ClassName, "MULTI") == 0:
			continue
		if j>11:
			Button = ClassWindow.GetControl(j+7)
		else:
			Button = ClassWindow.GetControl(j+2)

		t = CommonTables.ClassText.GetValue (ClassName, "LOWER")
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
	TextAreaControl.SetText (CommonTables.ClassText.GetValue (ClassName, "DESCSTR"))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def NextPress(Window):
	Window.Close()
	#class	
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	Class = CommonTables.ClassText.GetValue (ClassName, "CLASSID")
	MyChar = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, Class)

	# gnomes are forced to specialize also when multiclassing
	if "MAGE" in ClassName and GemRB.GetVar ("MAGESCHOOL"):
		KitIndex = GemRB.GetVar ("MAGESCHOOL")
		SchoolList = GemRB.LoadTable ("magesch", 1)
		KitValue = SchoolList.GetValue (KitIndex, 3)
		GemRB.SetPlayerStat (MyChar, IE_KIT, KitValue)

	CharGenCommon.next()

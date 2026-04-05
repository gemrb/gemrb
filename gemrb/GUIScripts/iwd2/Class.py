# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, class (GUICG2)
import GemRB
import GUICommon
import CharOverview
import CommonTables
from GUIDefines import *
from ie_stats import IE_CLASS, IE_KIT

ClassWindow = 0
TextAreaControl = 0
DoneButton = 0
BackButton = 0
ClassCount = 0
HasSubClass = 0
ClassID = 0

def AdjustTextArea():
	global HasSubClass, ClassID

	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.ClassText.GetValue (ClassName, "DESCSTR"))
	ClassID = CommonTables.ClassText.GetValue (ClassName, "CLASSID")
	#determining if this class has any subclasses
	HasSubClass = 0
	for i in range(1, ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = CommonTables.Classes.GetValue(ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		HasSubClass = 1
		break

	if HasSubClass == 0:
		DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	else:
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	return

def OnLoad():
	global ClassWindow, TextAreaControl, DoneButton, BackButton
	global ClassCount

	#this replaces help02.2da for class restrictions
	ClassCount = CommonTables.Classes.GetRowCount()+1
	ClassWindow = GemRB.LoadWindow(2, "GUICG")
	CharOverview.PositionCharGenWin(ClassWindow)

	DoneButton = ClassWindow.GetControl (0)
	TextAreaControl = ClassWindow.GetControl (16)
	ClassWindow.SetEventProxy (TextAreaControl)
	BackButton = ClassWindow.GetControl (17)

	SetupClassList ()

def SetupClassList():
	RaceName = GUICommon.GetRaceRowName (GemRB.GetVar ("Slot"), GemRB.GetVar ("BaseRace"))

	#radiobutton groups must be set up before doing anything else to them
	j = 0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, "CLASS")
		if Allowed > 0: # skip subclasses
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)

	j = 0
	ClassRaceTable = GemRB.LoadTable ("clsrcreq")
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = CommonTables.Classes.GetValue(ClassName, "CLASS")
		if Allowed > 0:
			continue
		Allowed = ClassRaceTable.GetValue (ClassName, RaceName, GTV_INT)
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = CommonTables.ClassText.GetValue (ClassName, "LOWER")
		Button.SetText(t )

		if Allowed==0:
			continue
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.OnPress (ClassPress)
		Button.SetVarAssoc("Class", i)

	BackButton.SetText(15416)
	BackButton.MakeEscape()

	DoneButton.SetText(36789)
	DoneButton.MakeDefault()

	Class = GemRB.GetVar("Class")-1
	if Class<0:
		TextAreaControl.SetText(17242)
		DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	else:
		AdjustTextArea()

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	ClassWindow.Focus()
	return

def ClassPress():
	global HasSubClass

	AdjustTextArea()
	if HasSubClass == 0:
		return

	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	j = 0
	for i in range(1,ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		Allowed = CommonTables.Classes.GetValue(ClassName, "CLASS")
		if Allowed > 0:
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON, OP_SET)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText("")

	j=0
	for i in range(1, ClassCount):
		ClassName = CommonTables.Classes.GetRowName(i-1)
		#determining if this is a kit or class
		Allowed = CommonTables.Classes.GetValue(ClassName, "CLASS")
		if Allowed != ClassID:
			continue
		Button = ClassWindow.GetControl(j+2)
		j = j+1
		t = CommonTables.ClassText.GetValue (ClassName, "LOWER")
		Button.SetText(t )
		Button.SetState(IE_GUI_BUTTON_ENABLED)
		Button.OnPress (ClassPress2)
		Button.SetVarAssoc("Class", i)

	BackButton.OnPress (BackPress2)
	return

def ClassPress2():
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	TextAreaControl.SetText (CommonTables.ClassText.GetValue (ClassName, "DESCSTR"))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress2():
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	SetupClassList ()
	return

def BackPress():
	if ClassWindow:
		ClassWindow.Close ()
	GemRB.SetNextScript("CharGen3")
	GemRB.SetVar("Class",0)  #scrapping the class value
	MyChar = GemRB.GetVar("Slot")
	GemRB.SetPlayerStat (MyChar, IE_CLASS, 0)
	GemRB.SetPlayerStat (MyChar, IE_KIT, 0)
	return

def NextPress():
	#classcolumn is base class
	ClassName = GUICommon.GetClassRowName (GemRB.GetVar ("Class")-1, "index")
	ClassColumn = CommonTables.Classes.GetValue (ClassName, "CLASS")
	if ClassColumn <= 0:  #it was already a base class
		ClassColumn = GemRB.GetVar("Class")
	GemRB.SetVar("BaseClass", ClassColumn)
	if ClassWindow:
		ClassWindow.Close ()
	GemRB.SetNextScript("CharGen4") #alignment
	return

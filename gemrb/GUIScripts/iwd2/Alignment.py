# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, alignment (GUICG3)
import GemRB
import CharOverview
import CommonTables
from ie_stats import IE_ALIGNMENT
from GUIDefines import *

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	
	Class = GemRB.GetVar("Class")-1
	KitName = CommonTables.Classes.GetRowName(Class)

	AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	AlignmentWindow = GemRB.LoadWindow(3, "GUICG")
	CharOverview.PositionCharGenWin(AlignmentWindow)
	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText (CommonTables.Aligns.GetValue (i, 0))

		if AlignmentOk.GetValue (KitName, CommonTables.Aligns.GetValue (i, 4)) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.OnPress (AlignmentPress)
			Button.SetVarAssoc("Alignment", i+1)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.MakeEscape()

	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(36789)
	DoneButton.MakeDefault()

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)
	AlignmentWindow.SetEventProxy (TextAreaControl)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.Focus()
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")-1
	TextAreaControl.SetText (CommonTables.Aligns.GetValue (Alignment, 1))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Close ()
	GemRB.SetNextScript("CharGen4")
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	pc = GemRB.GetVar ("Slot")
	GemRB.SetPlayerStat (pc, IE_ALIGNMENT, 0)
	return

def NextPress():
	if AlignmentWindow:
		AlignmentWindow.Close ()
	GemRB.SetNextScript("CharGen5") #appearance

	pc = GemRB.GetVar ("Slot")
	idx = GemRB.GetVar ("Alignment") - 1
	GemRB.SetPlayerStat (pc, IE_ALIGNMENT, CommonTables.Aligns.GetValue (idx, 3))
	return

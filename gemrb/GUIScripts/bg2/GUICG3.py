# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, alignment (GUICG3)
import GemRB
import GUICommon
import CommonTables
from ie_stats import *
from GUIDefines import *

import CharGenCommon

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
MyChar = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global MyChar
	
	MyChar = GemRB.GetVar ("Slot")
	KitName = GUICommon.GetKitRowName (MyChar)
	AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	AlignmentWindow = GemRB.LoadWindow(3, "GUICG")
	CharGenCommon.PositionCharGenWin(AlignmentWindow)

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText (CommonTables.Aligns.GetValue (i,0))

		if AlignmentOk.GetValue(KitName, CommonTables.Aligns.GetValue (i, 4)) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.OnPress (AlignmentPress)
			Button.SetVarAssoc("Alignment", i)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.MakeEscape()
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.MakeDefault()

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.OnPress (NextPress)
	BackButton.OnPress (BackPress)
	DoneButton.SetDisabled(True)
	AlignmentWindow.Focus()
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")
	TextAreaControl.SetText (CommonTables.Aligns.GetValue (Alignment, 1))
	DoneButton.SetDisabled(False)
	GemRB.SetVar ("Alignment", CommonTables.Aligns.GetValue (Alignment, 3))
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Close ()
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	GemRB.SetNextScript("CharGen4")
	return

def NextPress():
	if AlignmentWindow:
		AlignmentWindow.Close ()
	# save previous stats:
	#       alignment
	Alignment = GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment)

	GemRB.SetNextScript("CharGen5") #appearance
	return

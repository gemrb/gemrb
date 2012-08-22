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
#character generation, alignment (GUICG3)
import GemRB
from GUIDefines import *
from ie_stats import *
import CharGenCommon
import GUICommon
import CommonTables


AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton

	if GUICommon.CloseOtherWindow (OnLoad):
		if(AlignmentWindow):
			AlignmentWindow.Unload()
			AlignmentWindow = None
		return

	MyChar = GemRB.GetVar ("Slot")

	GemRB.SetVar("Alignment",-1)
	
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	ClassRow = CommonTables.Classes.FindValue(5,Class)
	KitName = CommonTables.Classes.GetRowName(ClassRow)

	AlignmentOk = GemRB.LoadTable("ALIGNMNT")

	GemRB.LoadWindowPack("GUICG", 640, 480)
	AlignmentWindow = GemRB.LoadWindow(3)

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText (CommonTables.Aligns.GetValue (i,0))

	# This section enables or disables different alignment selections
	# based on Class, and depends on the ALIGNMNT.2DA table
	#
	# For now, we just enable all buttons
	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		if AlignmentOk.GetValue (KitName, CommonTables.Aligns.GetValue (i, 4)) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
		else:
			Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, AlignmentPress)
		Button.SetVarAssoc("Alignment", i)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)


	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, NextPress)
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CharGenCommon.BackPress)
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.ShowModal(MODAL_SHADOW_NONE)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")
	TextAreaControl.SetText (CommonTables.Aligns.GetValue (Alignment,1))
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	GemRB.SetVar ("Alignment", CommonTables.Aligns.GetValue (Alignment,3))
	return

def NextPress():
	MyChar = GemRB.GetVar ("Slot")
	Alignment = GemRB.GetVar ("Alignment")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment)
	CharGenCommon.next()

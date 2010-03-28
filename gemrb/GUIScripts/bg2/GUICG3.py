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
from GUICommon import *

AlignmentWindow = 0
TextAreaControl = 0
DoneButton = 0
AlignmentTable = 0
MyChar = 0

def OnLoad():
	global AlignmentWindow, TextAreaControl, DoneButton
	global AlignmentTable, MyChar
	
	MyChar = GemRB.GetVar ("Slot")
	Kit = GetKitIndex (MyChar)
	Class = GemRB.GetPlayerStat (MyChar, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	if Kit == 0:
		KitName = ClassTable.GetRowName(Class)
	else:
		#rowname is just a number, first value row what we need here
		KitName = KitListTable.GetValue(Kit, 0)

	AlignmentOk = GemRB.LoadTableObject("ALIGNMNT")

	GemRB.LoadWindowPack("GUICG", 640, 480)
	AlignmentTable = GemRB.LoadTableObject("aligns")
	AlignmentWindow = GemRB.LoadWindowObject(3)

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		Button.SetFlags(IE_GUI_BUTTON_RADIOBUTTON,OP_OR)
		Button.SetState(IE_GUI_BUTTON_DISABLED)
		Button.SetText(AlignmentTable.GetValue(i,0) )

	for i in range(9):
		Button = AlignmentWindow.GetControl(i+2)
		if AlignmentOk.GetValue(KitName, AlignmentTable.GetValue(i, 4) ) != 0:
			Button.SetState(IE_GUI_BUTTON_ENABLED)
			Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, "AlignmentPress")
			Button.SetVarAssoc("Alignment", i)

	BackButton = AlignmentWindow.GetControl(13)
	BackButton.SetText(15416)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	DoneButton = AlignmentWindow.GetControl(0)
	DoneButton.SetText(11973)
	DoneButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	TextAreaControl = AlignmentWindow.GetControl(11)
	TextAreaControl.SetText(9602)

	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"NextPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS,"BackPress")
	DoneButton.SetState(IE_GUI_BUTTON_DISABLED)
	AlignmentWindow.SetVisible(WINDOW_VISIBLE)
	return

def AlignmentPress():
	Alignment = GemRB.GetVar("Alignment")
	TextAreaControl.SetText(AlignmentTable.GetValue(Alignment,1) )
	DoneButton.SetState(IE_GUI_BUTTON_ENABLED)
	GemRB.SetVar("Alignment",AlignmentTable.GetValue(Alignment,3) )
	return

def BackPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	GemRB.SetVar("Alignment",-1)  #scrapping the alignment value
	GemRB.SetNextScript("CharGen4")
	return

def NextPress():
	if AlignmentWindow:
		AlignmentWindow.Unload()
	# save previous stats:
	#       alignment
	#       reputation
	#       alignment abilities
	Alignment = GemRB.GetVar ("Alignment")
	AlignmentTable = GemRB.LoadTableObject ("aligns")
	GemRB.SetPlayerStat (MyChar, IE_ALIGNMENT, Alignment)

	# use the alignment to apply starting reputation
	RepTable = GemRB.LoadTableObject ("repstart")
	AlignmentAbbrev = AlignmentTable.FindValue (3, Alignment)
	Rep = RepTable.GetValue (AlignmentAbbrev, 0) * 10
	GemRB.SetPlayerStat (MyChar, IE_REPUTATION, Rep)

	# set the party rep if this in the main char
	if MyChar == 1:
		GemRB.GameSetReputation (Rep)

	# diagnostic output
	print "CharGen5 output:"
	print "\tAlignment: ",Alignment
	print "\tReputation: ",Rep

	GemRB.SetNextScript("CharGen5") #appearance
	return

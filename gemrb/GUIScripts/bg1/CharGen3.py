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
# $Id$
#
#character generation (GUICG 0)
import GemRB

CharGenWindow = 0
TextAreaControl = 0

def OnLoad():
	global CharGenWindow, TextAreaControl

	GemRB.SetVar("Class",0) #class
	#GemRB.SetVar("Class Kit",0) #class kit

	GemRB.LoadWindowPack("GUICG")
	CharGenWindow = GemRB.LoadWindowObject(0)
	PortraitButton = CharGenWindow.GetControl(12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	PortraitButton.SetPicture (PortraitName,"NOPORTLG")

	RaceTable = GemRB.LoadTableObject("races")

	GenderButton = CharGenWindow.GetControl(0)
	GenderButton.SetText(11956)
	GenderButton.SetState(IE_GUI_BUTTON_DISABLED)

	RaceButton = CharGenWindow.GetControl(1)
	RaceButton.SetText(11957)
	RaceButton.SetState(IE_GUI_BUTTON_DISABLED)

	ClassButton = CharGenWindow.GetControl(2)
	ClassButton.SetText(11959)
	ClassButton.SetState(IE_GUI_BUTTON_ENABLED)
	ClassButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)

	AlignmentButton = CharGenWindow.GetControl(3)
	AlignmentButton.SetText(11958)
	AlignmentButton.SetState(IE_GUI_BUTTON_DISABLED)

	AbilitiesButton = CharGenWindow.GetControl(4)
	AbilitiesButton.SetText(11960)
	AbilitiesButton.SetState(IE_GUI_BUTTON_DISABLED)

	SkillButton = CharGenWindow.GetControl(5)
	SkillButton.SetText(17372)
	SkillButton.SetState(IE_GUI_BUTTON_DISABLED)

	AppearanceButton = CharGenWindow.GetControl(6)
	AppearanceButton.SetText(11961)
	AppearanceButton.SetState(IE_GUI_BUTTON_DISABLED)

	NameButton = CharGenWindow.GetControl(7)
	NameButton.SetText(11963)
	NameButton.SetState(IE_GUI_BUTTON_DISABLED)

	BackButton = CharGenWindow.GetControl(11)
	#BackButton.SetText(15416)
	BackButton.SetState(IE_GUI_BUTTON_ENABLED)

	AcceptButton = CharGenWindow.GetControl(8)
	AcceptButton.SetText(11962)
	AcceptButton.SetState(IE_GUI_BUTTON_DISABLED)

	ImportButton = CharGenWindow.GetControl(13)
	ImportButton.SetText(13955)
	ImportButton.SetState(IE_GUI_BUTTON_ENABLED)

	CancelButton = CharGenWindow.GetControl(15)
	CancelButton.SetText(13727)
	CancelButton.SetState(IE_GUI_BUTTON_ENABLED)

	TextAreaControl= CharGenWindow.GetControl(9)
	TextAreaControl.SetText(12135)
	TextAreaControl.Append(": ")
	if GemRB.GetVar("Gender") == 1:
		TextAreaControl.Append(1050)
	else:
		TextAreaControl.Append(1051)
	TextAreaControl.Append(1048,-1) # new line
	TextAreaControl.Append(": ")
	TextAreaControl.Append(RaceTable.GetValue(GemRB.GetVar("Race")-1,2))

	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	BackButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "BackPress")
	ClassButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "NextPress")
	ImportButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ImportPress")
	CharGenWindow.SetVisible(1)
	return
	
def NextPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("GUICG2") #class
	return

def CancelPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("CharGen")
	return

def BackPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("CharGen2") #appearance
	return

def ImportPress():
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetToken("NextScript","CharGen3")
	GemRB.SetNextScript("ImportFile") #import
	return

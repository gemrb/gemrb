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
#feedback
import GemRB

FeedbackWindow = 0
TextAreaControl = 0

def OnLoad():
	global FeedbackWindow, TextAreaControl

	GemRB.LoadWindowPack("GUIOPT", 640, 480)
	FeedbackWindow = GemRB.LoadWindowObject(9)

	MarkerSlider = FeedbackWindow.GetControl(30)
	MarkerSliderS = FeedbackWindow.GetControl(8)

	LocatorSlider = FeedbackWindow.GetControl(31)
	LocatorSliderS = FeedbackWindow.GetControl(9)

	THac0Rolls = FeedbackWindow.GetControl(32)
	THac0RollsB = FeedbackWindow.GetControl(10)

	CombatInfo = FeedbackWindow.GetControl(33)
	CombatInfoB = FeedbackWindow.GetControl(11)

	Actions = FeedbackWindow.GetControl(34)
	ActionsB = FeedbackWindow.GetControl(12)

	StateChanges = FeedbackWindow.GetControl(35)
	StateChangesB = FeedbackWindow.GetControl(13)

	SelectionText = FeedbackWindow.GetControl(36)
	SelectionTextB = FeedbackWindow.GetControl(14)

	Miscellaneous = FeedbackWindow.GetControl(37)
	MiscellaneousB = FeedbackWindow.GetControl(15)
	OkButton = FeedbackWindow.GetControl(26)
	CancelButton = FeedbackWindow.GetControl(27)
	TextAreaControl = FeedbackWindow.GetControl(28)

	TextAreaControl.SetText(18043)
	OkButton.SetText(11973)
	CancelButton.SetText(13727)
	
	MarkerSlider.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MarkerSliderPress")
	MarkerSliderS.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "MarkerSliderPress")
	MarkerSliderS.SetVarAssoc("GUI Feedback Level",1)

	LocatorSlider.SetEvent(IE_GUI_BUTTON_ON_PRESS, "LocatorSliderPress")
	LocatorSliderS.SetEvent(IE_GUI_SLIDER_ON_CHANGE, "LocatorSliderPress")
	LocatorSliderS.SetVarAssoc("Locator Feedback Level",1)

	THac0Rolls.SetEvent(IE_GUI_BUTTON_ON_PRESS, "THac0RollsPress")
	THac0RollsB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "THac0RollsBPress")
	THac0RollsB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	THac0RollsB.SetVarAssoc("Rolls",1)

	CombatInfo.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CombatInfoPress")
	CombatInfoB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CombatInfoBPress")
	CombatInfoB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	CombatInfoB.SetVarAssoc("Combat Info",1)

	Actions.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionsPress")
	ActionsB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "ActionsBPress")
	ActionsB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	ActionsB.SetVarAssoc("Actions",1)

	StateChanges.SetEvent(IE_GUI_BUTTON_ON_PRESS, "StateChangesPress")
	StateChangesB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "StateChangesBPress")
	StateChangesB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	StateChangesB.SetVarAssoc("State Changes",1)

	SelectionText.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionTextPress")
	SelectionTextB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "SelectionTextBPress")
	SelectionTextB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	SelectionTextB.SetVarAssoc("Selection Text",1)

	Miscellaneous.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MiscellaneousPress")
	MiscellaneousB.SetEvent(IE_GUI_BUTTON_ON_PRESS, "MiscellaneousBPress")
	MiscellaneousB.SetFlags(IE_GUI_BUTTON_CHECKBOX, OP_OR)
	MiscellaneousB.SetVarAssoc("Miscellaneous Text",1)

	OkButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "OkPress")
	OkButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
	CancelButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, "CancelPress")
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
	FeedbackWindow.SetVisible(1)
	return

def MarkerSliderPress():
	TextAreaControl.SetText(18024)
	return

def LocatorSliderPress():
	TextAreaControl.SetText(18025)
	return

def THac0RollsPress():
	TextAreaControl.SetText(18026)
	return

def CombatInfoPress():
	TextAreaControl.SetText(18027)
	return

def ActionsPress():
	TextAreaControl.SetText(18028)
	return

def StateChangesPress():
	TextAreaControl.SetText(18029)
	return

def SelectionTextPress():
	TextAreaControl.SetText(18030)
	return

def MiscellaneousPress():
	TextAreaControl.SetText(18031)
	return

def OkPress():
	if FeedbackWindow:
		FeedbackWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return

def CancelPress():
	if FeedbackWindow:
		FeedbackWindow.Unload()
	GemRB.SetNextScript("GUIOPT8")
	return


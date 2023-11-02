# GemRB - Infinity Engine Emulator
# Copyright (C) 2012 The GemRB Project
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# GUIOPTControls.py - control functions for the GUIOPT winpack:

###################################################

import GemRB
import GameCheck
from GUIDefines import *

###################################################
# strrefs
if GameCheck.IsPST():
	STR_OPT_DONE = 1403
	STR_OPT_CANCEL = 4196
else:
	STR_OPT_DONE = 11973
	STR_OPT_CANCEL = 13727

###################################################
# These functions help to setup controls found
# in Video, Audio, Gameplay, Feedback and Autopause
# options windows

# These controls are usually made from an active
# control (button, slider ...) and a label

# NOTE: make sure handler users also set the strref in them!
def OptSlider (winhelp, ctlhelp, slider_id, label_id, label_strref, variable, action = None, value = 1):
	"""Standard slider for option windows"""

	window = GetWindow ()
	slider = window.GetControl (slider_id)
	helpTA = GemRB.GetView ("OPTHELP")
	slider.SetVarAssoc (variable, value)
	if action:
		slider.OnChange (action)
	else:
		# create an anonymous callback, so we don't need to create a separate function for each string
		slider.OnChange (lambda: helpTA.SetText (ctlhelp))

	OptBuddyLabel (label_id, label_strref, ctlhelp, winhelp)
	if HasMouseOver ():
		slider.OnMouseEnter (lambda: helpTA.SetText (ctlhelp))
		slider.OnMouseLeave (lambda: helpTA.SetText (winhelp))

	return slider

def OptRadio (action, button_id, label_id, variable, value, label_strref = None, focusedText = None, defaultText = None):
	"""Standard radio button for option windows"""

	window = GetWindow ()
	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	button.OnPress (action)
	button.SetVarAssoc (variable, value)
	if GameCheck.IsIWD2():
		button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	elif GameCheck.IsIWD1() or GameCheck.IsBG1():
		button.SetSprites ("TOGGLE", 0, 0, 1, 3, 2)

	OptBuddyLabel (label_id, label_strref, focusedText, defaultText)

	return button

# NOTE: make sure handler users also set the strref in them!
def OptCheckbox (winhelp, ctlhelp, button_id, label_id, label_strref, variable, handler = None, value = 1):
	"""Standard checkbox for option windows"""

	window = GetWindow ()
	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	if variable:
		button.SetVarAssoc (variable, value)

	if GameCheck.IsIWD2():
		button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	elif GameCheck.IsIWD1() or GameCheck.IsBG1():
		button.SetSprites ("GMPPARBC", 3, 1, 2, 3, 5)

	def callback():
		GemRB.GetView ("OPTHELP").SetText (ctlhelp)

	if handler:
		button.OnPress (handler)
	elif HasMouseOver ():
		button.OnMouseEnter (callback)
	else:
		button.OnPress (callback)

	OptBuddyLabel (label_id, label_strref, ctlhelp, winhelp)

	return button

def OptButton (action, button_id, button_strref):
	"""Standard subwindow button for option windows"""

	window = GetWindow ()
	button = window.GetControl (button_id)
	button.OnPress (action)
	button.SetText (button_strref)

def OptDone (action, button_id):
	"""Standard `Done' button for option windows"""

	window = GetWindow ()
	button = window.GetControl (button_id)
	button.SetText (STR_OPT_DONE) # Done
	button.OnPress (action)
	button.MakeDefault()

	if GameCheck.IsPST():
		button.SetVarAssoc ("Cancel", 0)

def OptCancel (action, button_id):
	"""Standard `Cancel' button for option windows"""

	window = GetWindow ()
	button = window.GetControl (button_id)
	button.SetText (STR_OPT_CANCEL) # Cancel
	button.OnPress (action)
	button.MakeEscape()

	if GameCheck.IsPST():
		button.SetVarAssoc ("Cancel", 1)

def OptHelpText (name, text_id, text_strref):
	"""Standard textarea with context help for option windows"""

	window = GetWindow ()
	text = window.GetControl (text_id)
	text.SetText (text_strref)
	text.AddAlias ("OPTHELP", 0, 1)
	return text

def OptBuddyLabel (label_id, label_strref = None, focusedText = None, defaultText = None):
	window = GetWindow ()
	label = window.GetControl (label_id)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)
	if label_strref and GameCheck.IsPST():
		label.SetText (label_strref)
	help_ta = GemRB.GetView ("OPTHELP")
	if help_ta and HasMouseOver ():
		label.OnMouseEnter (lambda: help_ta.SetText (focusedText))
		label.OnMouseLeave (lambda: help_ta.SetText (defaultText))
	else:
		label.OnPress (lambda: help_ta.SetText (focusedText))

def GetWindow ():
	win = GemRB.GetView ("SUB_WIN", 1)
	return win if win else GemRB.GetView ("SUB_WIN", 0)

def HasMouseOver ():
	return GameCheck.IsPST () or GameCheck.IsIWD2 ()

###################################################
# End of file GUIOPTControls.py

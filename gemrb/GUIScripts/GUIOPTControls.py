# -*-python-*-
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
import GUICommon
from GUIDefines import *

###################################################
# strrefs
if GUICommon.GameIsPST():
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

def OptSlider (action, window, slider_id, variable, value):
	"""Standard slider for option windows"""
	slider = window.GetControl (slider_id)
	slider.SetVarAssoc (variable, value)
	slider.SetEvent (IE_GUI_SLIDER_ON_CHANGE, action)
	return slider

def OptSliderNoCallback (strref, help_ta, window, slider_id, variable, value):
	"""Standard slider for option windows, but without a custom callback"""
	slider = window.GetControl (slider_id)
	slider.SetVarAssoc (variable, value)
	# create an anonymous callback, so we don't need to create a separate function for each string
	slider.SetEvent (IE_GUI_SLIDER_ON_CHANGE, lambda s=strref, ta=help_ta: ta.SetText (s))
	return slider

def OptRadio (action, window, button_id, label_id, variable, value):
	"""Standard radio button for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, action)
	button.SetVarAssoc (variable, value)
	if GUICommon.GameIsIWD2():
		button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	OptBuddyLabel (window, label_id)

	return button

# NOTE: make sure handler users also set the strref in them!
def OptCheckbox (winhelp, ctlhelp, help_ta, window, button_id, label_id, label_strref, variable, handler=None, value=1):
	"""Standard checkbox for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	if variable:
		button.SetVarAssoc (variable, value)

	if GUICommon.GameIsIWD2():
		button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)
	elif GUICommon.GameIsIWD1():
		button.SetSprites ("GMPPARBC", 3, 1, 2, 3, 5)

	if handler:
		button.SetEvent (IE_GUI_BUTTON_ON_PRESS, handler)
	else:
		# create an anonymous callback, so we don't need to create a separate function for each string
		# FIXME: IE_GUI_MOUSE_ENTER_BUTTON would be more UX-sensible, but interferes with toggling
		button.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda s=ctlhelp, ta=help_ta: ta.SetText (s))

	OptBuddyLabel (window, label_id, label_strref, help_ta, ctlhelp, winhelp)

	return button

def OptButton (action, window, button_id, button_strref):
	"""Standard subwindow button for option windows"""

	button = window.GetControl (button_id)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, action)
	button.SetText (button_strref)

def OptDone (action, window, button_id):
	"""Standard `Done' button for option windows"""

	button = window.GetControl (button_id)
	button.SetText (STR_OPT_DONE) # Done
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, action)
	button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	if GUICommon.GameIsPST():
		button.SetVarAssoc ("Cancel", 0)

def OptCancel (action, window, button_id):
	"""Standard `Cancel' button for option windows"""

	button = window.GetControl (button_id)
	button.SetText (STR_OPT_CANCEL) # Cancel
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, action)
	button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	if GUICommon.GameIsPST():
		button.SetVarAssoc ("Cancel", 1)

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = window.GetControl (text_id)
	text.SetText (text_strref)
	return text

def OptBuddyLabel (window, label_id, label_strref = None, help_ta = None, ctlname = None, winname = None):
	label = window.GetControl (label_id)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)
	if label_strref and GUICommon.GameIsPST():
		label.SetText (label_strref)
	if help_ta:
		label.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, lambda: help_ta.SetText (ctlname))
		label.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, lambda: help_ta.SetText (winname))

###################################################
# End of file GUIOPTControls.py

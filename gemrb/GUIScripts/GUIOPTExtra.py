# GemRB - Infinity Engine Emulator
# Copyright (C) 2025 The GemRB Project
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

# GUIOPTExtra.py - script to control some GemRB options at runtime
# it reuses the GUIOPT autopause window

import GemRB
import GameCheck
import GUIOPTControls
from GUIDefines import *

def OpenGemRBOptionsWindow ():
	# if we ever need more than one subwindow open a selector window here instead
	OpenGUIEnhancementsWindow ()
	return

def AddGemRBOptionsButton (window, frame, xOff, yOff, sprite, cycle = 0):
	extraOpts = window.CreateButton (50, frame["x"] + xOff, frame["y"] + yOff, frame["w"], frame["h"])

	if GameCheck.IsBG2EE ():
		if sprite == "GUIOSTCL": # can't do this in the caller
			extraOpts = window.ReparentSubview (extraOpts, window.GetControl (12))
		extraOpts.SetFlags (IE_GUI_BUTTON_CAPS, OP_OR)
		extraOpts.SetText ("GemRB   options")
	else:
		if GameCheck.IsBG2 ():
			extraOpts.SetFlags (IE_GUI_BUTTON_CAPS, OP_OR)
		extraOpts.SetText ("GemRB options")

	if GameCheck.IsIWD1 () or GameCheck.IsIWD2 ():
		extraOpts.SetSprites (sprite, cycle, 1, 2, 0, 3)
	else: # PST, bg2ee, bg2
		extraOpts.SetSprites (sprite, cycle, 0, 1, 2, 3)

	extraOpts.OnPress (OpenGemRBOptionsWindow)
	return

def CloseConfigWindow ():
	GemRB.GetView ("SUB_WIN", 1).Close()
	GemRB.SaveConfig ()

def OpenGUIEnhancementsWindow ():
	title = [
		"",
		"New color for empty containers",
		"Autoidentify on item transfer",
		"Open bag contents directly",
		"Stop scrolling on focus loss",
		"Outline learnable scrolls",
		"Remember already identified items",
		"",
		""
	]

	desc = [
		"Here you can toggle various little improvements over the originals.",
		"Chests, stashes and other containers will be drawn in grey when highlighted instead of the default color. This makes it easy to distinguish those you have already fully looted.",
		"When moving items between pcs or containers, plus when opening an item description, try autoidentifying the item first to save a few clicks. It's still based on lore.",
		"Skip displaying the bag/quiver/ammo belt description window and open their item list immediately instead.",
		"When the mouse leaves the GemRB window, stop any area scrolling that was triggered near the border.",
		"Lightly outline inventory slots with spell scrolls that can still be learned from.",
		"Within the same session, remember any items that were identified. This means that if another of its kind is found, it will be autoidentified. And if you already have several copies, only identifying one is needed.",
		"",
		""
	]

	controlIDs = { 'win' : 10, 'help' : 15, 'done' : 11, 'cancel' : 14,
							 '1' : 1, '2' : 2, '3' : 3, '4' : 4,  '5' : 5, '6' : 13, '7' : 25, '8' : 26,
							 '1t' : 17, '2t' : 18, '3t' : 19, '4t' : 20,  '5t' : 21, '6t' : 22, '7t' : 24, '8t' : 27
							 }
	if GameCheck.IsPST ():
		controlIDs = { 'win' : 9, 'help' : 1, 'done' : 16, 'cancel' : 17,
								'1' : 2, '2' : 3, '3' : 4, '4' : 5,  '5' : 6, '6' : 7, '7' : 8, '8' : None,
								'1t' : 9, '2t' : 10, '3t' : 11, '4t' : 12,  '5t' : 13, '6t' : 14, '7t' : 15, '8t' : None
								}

	Window = GemRB.LoadWindow (controlIDs["win"], "GUIOPT")
	Window.AddAlias("SUB_WIN", 1)
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	Title = Window.GetControl (-1 + 0x10000000)
	titleText = "GemRB GUI enhancements"
	if GameCheck.IsPST () or GameCheck.IsBG1():
		titleText = titleText.upper()
	Title.SetText (titleText)

	# NOTE: since the second param is not a strref, reverting to default won't work
	GUIOPTControls.OptHelpText (controlIDs["help"], desc[0])

	GUIOPTControls.OptDone (CloseConfigWindow, controlIDs["done"])
	GUIOPTControls.OptCancel (Window.Close, controlIDs["cancel"])

	GUIOPTControls.OptCheckbox (desc[1], controlIDs["1"], controlIDs["1t"], title[1], 'GUIEnhancements', None, GE_CONTAINERS)
	GUIOPTControls.OptCheckbox (desc[2], controlIDs["2"], controlIDs["2t"], title[2], 'GUIEnhancements', None, GE_TRY_IDENTIFY_ON_TRANSFER)
	GUIOPTControls.OptCheckbox (desc[3], controlIDs["3"], controlIDs["3t"], title[3], 'GUIEnhancements', None, GE_ALWAYS_OPEN_CONTAINER_ITEMS)
	GUIOPTControls.OptCheckbox (desc[4], controlIDs["4"], controlIDs["4t"], title[4], 'GUIEnhancements', None, GE_UNFOCUS_STOPS_SCROLLING)
	GUIOPTControls.OptCheckbox (desc[5], controlIDs["5"], controlIDs["5t"], title[5], 'GUIEnhancements', None, GE_MARK_SCROLLS)
	GUIOPTControls.OptCheckbox (desc[6], controlIDs["6"], controlIDs["6t"], title[6], 'GUIEnhancements', None, GE_PERSISTENT_IDENTIFICATION)

	# only pst lacks the silly labels and sets the button text directly (otherwise there would be alignment issues)
	if not GameCheck.IsPST ():
		for cid in enumerate([5, 6, 7, 8, 9, 11, 22, 27, 28, 31]):
			label = Window.GetControl (cid[1] + 0x10000000)
			if label and cid[0] < 8:
				label.SetText (title[cid[0] + 1])
	# remove all the other entries
	# shift the ranges when new buttons will be needed
	for cid in (22 + 0x10000000, controlIDs["7"], controlIDs["7t"]):
		if Window.GetControl (cid):
			Window.DeleteControl (cid)
	for cid in range(24, 40):
		if Window.GetControl (cid):
			Window.DeleteControl (cid)
		if Window.GetControl (cid + 0x10000000):
			Window.DeleteControl (cid + 0x10000000)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

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

import configparser

import GemRB
import GameCheck
import GUIOPTControls
from GUIDefines import *

ScreenW = GemRB.GetSystemVariable (SV_WIDTH)
ScreenH = GemRB.GetSystemVariable (SV_HEIGHT)

# mapped controls of the GUIOPT window we reuse
controlIDs = { 'win' : 10, 'help' : 15, 'done' : 11, 'cancel' : 14,
							'1' : 1, '2' : 2, '3' : 3, '4' : 4,  '5' : 5, '6' : 13, '7' : 25, '8' : 26,
							'1t' : 17, '2t' : 18, '3t' : 19, '4t' : 20,  '5t' : 21, '6t' : 22, '7t' : 24, '8t' : 27
							}
if GameCheck.IsPST ():
	controlIDs = { 'win' : 9, 'help' : 1, 'done' : 16, 'cancel' : 17,
							'1' : 2, '2' : 3, '3' : 4, '4' : 5,  '5' : 6, '6' : 7, '7' : 8, '8' : None,
							'1t' : 9, '2t' : 10, '3t' : 11, '4t' : 12,  '5t' : 13, '6t' : 14, '7t' : 15, '8t' : None
							}

def OpenGemRBOptionsWindow ():
	# TODO: add hotkey config here for games that don't have windows for it (pst, iwd2, bg2 have GUIKEYS; bg1, iwd1 nothing; ees something else)
	#       or simplify and link those buttons to the same constructed window as well
	# if we ever need more than 3 buttons, switch to GUICONN 1 with 4/7/8 buttons (iwd2/bg1+iwd1/bg2) or add them manually (pst)
	Window = GemRB.LoadWindow (4 if GameCheck.IsPST () else 5, "GUIOPT")
	Button1 = Window.GetControl (0)
	Button1.OnPress (OpenGUIEnhancementsWindow)
	Button1.SetText ("Enhancements")
	Button1.MakeDefault ()

	Button2 = Window.GetControl (1)
	Button2.OnPress (OpenINIConfigWindow)
	Button2.SetText ("Hidden options")

	Back = Window.GetControl (2)
	Back.OnPress (Window.Close)
	Back.SetText ("Back")

	TextArea = Window.GetControl (3)
	TextArea.SetText ("Extra configuration windows and GemRB-only option setting.")

	Window.ShowModal (MODAL_SHADOW_GRAY)
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

def PrepareBlankWindow (title, introText, usedControls, usedPSTcontrols):
	Window = GemRB.LoadWindow (controlIDs["win"], "GUIOPT")
	Window.SetBackground ({'r' : 0, 'g' : 0, 'b' : 0, 'a' : 180})
	Window.SetFrame ({ 'x': 0, 'y': 0, 'h': ScreenH, 'w': ScreenW })
	Window.SetFlags (WF_BORDERLESS, OP_OR)
	Window.AddAlias ("SUB_WIN", 1)

	Title = Window.GetControl (-1 + 0x10000000)
	Title.SetFrame ({ 'x': 0, 'y': 10, 'h': 20, 'w': ScreenW })
	Title.SetText (title)
	GUIOPTControls.OptHelpText (controlIDs["help"], introText)
	GUIOPTControls.OptDone (CloseConfigWindow, controlIDs["done"])
	GUIOPTControls.OptCancel (Window.Close, controlIDs["cancel"])

	# delete all the unused controls
	for cid in range(1, 40):
		used = GameCheck.IsPST () and cid in usedPSTcontrols
		used = used or (not GameCheck.IsPST () and cid in usedControls)
		if used:
			if Window.GetControl (cid + 0x10000000):
				Window.DeleteControl (cid + 0x10000000)
			continue
		if Window.GetControl (cid + 0x10000000):
			Window.DeleteControl (cid + 0x10000000)
		if Window.GetControl (cid):
			Window.DeleteControl (cid)

	return Window

def PrepareINIKeyList ():
	iniMeta = GemRB.LoadTable ("inimeta", False, True)
	coolKeys = []
	for ri in range(iniMeta.GetRowCount ()):
		key = iniMeta.GetRowName (ri)
		controlType = iniMeta.GetValue (key, "CONTROL")
		cleanKey = key.replace("_", " ")
		coolKeys.append([cleanKey, controlType, -1, ""])

	# load ini
	config = configparser.ConfigParser (inline_comment_prefixes=(';'))
	config.optionxform = str # disable lowercasing
	GemRB.SaveConfig () # to ensure the files already exist
	GamePath = GemRB.GetSystemVariable (SV_GAMEPATH)
	iniFile = GemRB.GetSystemVariable (SV_INICONF)
	config.read (GamePath + "/gem-" + iniFile)

	defaultControlType = "" # deliberately invalid value

	# construct a manually ordered list by combining our table and what's in the ini
	# cycle through, save values of interesting keys, add all other keys
	coolKeysKeys = [k[0] for k in coolKeys]
	ignoredSections = ["Movies", "Multiplayer"]
	for section in config.sections():
		if section in ignoredSections:
			continue

		for key in config[section]:
			if key in coolKeysKeys:
				coolKeys[coolKeysKeys.index(key)][2] = config[section][key]
			else:
				coolKeys.append([key, defaultControlType, config[section][key], ""])

	# add descriptions to interesting keys
	descs = {
		"Maximum Frame Rate" : "Important: this is not drawing FPS! This setting determines how quickly the game ticks its own clock. The default of 30 means 15 ticks per second. This influences effect application, script action execution, walking and in general speeds up or slows down the gameplay.",
		"Zoom Lock" : "Set to 0 to enable zooming in and out with the mouse wheel. Middle button click resets the zoom level.",
		"Cheats" : "Toggle the availability of the debug console accessible with ctrl-space and cheat keys.",
		"Suppress Extra Difficulty Damage" : "Scale up enemy damage at higher difficulty levels?",
		"Maximum HP" : "Roll HP on level up or use the maximum the dice allow?",
		"Heal Party on Rest" : "When resting outside, do you want to rest until fully healed? Healing spells will be cast every 8 hours, but this can still make you sleep for hundreds of days. You can fail quests that rely on timers.",
		"Infravision" : "Color actors detected by infravision differently?",
		"3E Thief Sneak Attack" : "Use sneak attack damage calculations instead of the backstab multiplier?",
		"Display Movie Subtitles" : "Display subtitles in movies? Same as Display Subtitles.",
		"Display Subtitles" : "Same as Display Movie Subtitles.",
		"Duplicate Floating Text" : "Duplicate any floating text to the message window?",
		"Subtitles" : "Print character quotes (from their voice set) to the message window? Otherwise only the audio will play. Can also be used to override the two movie subtitle settings if they are disabled.",
		"Bored Timeout" : "How often do pcs get bored and comment on it or area specifics.",
		"Hotkeys On Tooltips" : "Should button tooltips show their associated hotkey?",
		"HP Over Head" : "Always display PC hitpoints over their heads, not just when pressing TAB?",
		"EnableRollFeedback" : "IWD2: the optional detailed feedback on combat and action rolls.",
		"Strref On" : "Debug option to prefix every string with its string reference from dialog.tlk.",
		"DefaultCharGenPointsPool" : "IWD2: the starting points to distribute when creating characters.",
		}

	coolKeysKeys = [k[0] for k in coolKeys]
	for desc in descs:
		if not desc in coolKeysKeys:
			continue
		coolKeys[coolKeysKeys.index(desc)][3] = descs[desc]
	return coolKeys

def GuessControlType (keyData):
	key, controlType, value, _ = keyData
	if controlType != "":
		return controlType

	# special cases, since the value could get set to 0
	special = [ "Bored Timeout", "GUIEnhancements", "Mouse Scroll Speed", "Keyboard Scroll Speed",
						"Command Sound Frequency", "Selection Sound Frequency", "Auto Pause State", "Gamma Correction" ]
	if key in special or "Volume" in key or "Level" in key:
		return "textedit"
	if value == "0" or value == "1":
		return "checkbox"
	return "textedit"

# create a line of controls for a given ini key
# a button to host the label and a control to adjust the value
def AddKeyControls (window, keyData, position, offset, introText):
	# return
	key, controlType, value, desc = keyData
	controlType = GuessControlType (keyData)
	firstID = offset["c"] * 1000 + offset["r"]
	secondID = offset["c"] * 10000 + offset["r"]

	# create button to display key name and help on click
	desc = introText if desc == "" else desc
	labelW = 200 if offset["c"] == 2 else 250
	labelBtn = window.CreateButton (firstID, position["x"], position["y"], labelW, 20)
	GUIOPTControls.OptBuddyLabel (firstID, None, desc, introText)
	labelBtn.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT, OP_OR)
	labelBtn.SetText (key)
	btnFrame = labelBtn.GetFrame ()

	if controlType == "none":
		# just a spacer, a label is enough
		if key == "Spacer":
			labelBtn.SetText (None)
		labelBtn.SetState (IE_GUI_BUTTON_DISABLED) # tooltip fix
		return
	elif controlType == "checkbox":
		secondCtrl = window.CreateButton (secondID, btnFrame["x"] + btnFrame["w"] + 10, position["y"], 40, 25)
		secondCtrl = GUIOPTControls.OptCheckbox (desc, secondID, None, None, key)
	elif controlType == "textedit":
		# fake a TextEdit; replace with a GUIOPTControls method if it ever gets added
		secondCtrl = window.CreateTextArea (secondID, btnFrame["x"] + btnFrame["w"] + 10, position["y"] + 5, 50, 20, "NORMAL")
		secondCtrl.SetFlags (IE_GUI_TEXTAREA_EDITABLE, OP_OR)
		secondCtrl.SetColor (ColorWhitish, TA_COLOR_NORMAL)
		secondCtrl.SetText (value)
		secondCtrl.OnChange (lambda: GemRB.SetVar (key, int(secondCtrl.QueryText())))
	else:
		print("Unhandled inimeta.2da control type: " + controlType)
		return

	return

# window for configuring ini keys, mainly for those that are not covered by the original GUIs
def OpenINIConfigWindow ():
	# curate a partially sorted and grouped list of interesting config keys, so
	# we can present them first. All the others follow afterwards
	iniKeys = PrepareINIKeyList ()

	# prepare a blank full-screen window
	# make sure not to delete the scrollbar attached to the textarea!
	iniFile = GemRB.GetSystemVariable (SV_INICONF)
	title = "Detailed configuration from gem-" + iniFile
	introText = "Here you can change various settings that may otherwise not be exposed in the options window. It's the same as editing the [color=0xff0000]gem-%s[/color] file directly.\n\nFirst some of the more interesting options are listed, then every other that appears in the file, except for the list of already seen movies. Some may require a restart to truly take effect." %(iniFile)
	# keep the done and cancel buttons, title label, textarea and associated scrollbar (non-pst)
	Window = PrepareBlankWindow (title, introText, [11, 14, 15, 16], [1, 16, 17])

	# move help textarea to the top and widen, the rest of the screen will be for the main controls
	# NOTE: if the window is small enough, the user could scroll this textarea out of view — consider
	# nesting the main controls in another textarea to contain it within the window instead
	HelpTA = GemRB.GetView ("OPTHELP")
	if ScreenW > 800:
		HelpTA.SetFrame ({ 'x': (ScreenW - 800) // 2, 'y': 50, 'h': 80, 'w': 800 })
	else:
		HelpTA.SetFrame ({ 'x': 10, 'y': 50, 'h': 80, 'w': ScreenW - 20 })
	HelpTA.Scroll (0, 0)

	# construct a set of controls for each ini key
	newRowH = 25
	newRowW = 250 + 10 + 50
	topStart = 150 # title + help + gaps
	if ScreenW >= 800:
		xSingleRowOffset = ScreenW // 2 - newRowW - 30
	else:
		xSingleRowOffset = ScreenW // 2 - 2 * newRowW // 3
	i = 0
	keyCount = len(iniKeys)
	deepestPosition =  0
	for keyData in iniKeys:
		# add controls to up to two columns
		if i > keyCount // 2 and ScreenW >= 800:
			offset = { "c": 2, "r": i - keyCount // 2 }
			position = { "x": ScreenW // 2, "y": topStart + newRowH * offset["r"] }
		else:
			offset = { "c": 1, "r": i }
			position = { "x": xSingleRowOffset, "y": topStart + newRowH * offset["r"] }
		if position["y"] > deepestPosition:
			deepestPosition = position["y"]

		AddKeyControls(Window, keyData, position, offset, introText)
		i += 1

	# reposition done and cancel at the bottom center
	yDone = deepestPosition + newRowH + 30
	button = Window.GetControl (16 if GameCheck.IsPST () else 11)
	btnFrame = button.GetFrame ()
	button.SetPos (ScreenW // 2 + 10, yDone)
	button = Window.GetControl (17 if GameCheck.IsPST () else 14)
	button.SetPos (ScreenW // 2 - btnFrame["w"] - 10, yDone)
	# HACK: create another control below to force a resize of the scrollview
	# a move is not enough and resizing the window itself would remove the scrollbar
	# we need some margin  any way
	Window.CreateButton (123, 0, yDone + btnFrame["h"] + 30, 5, 5)

	Window.ShowModal (MODAL_SHADOW_GRAY)

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

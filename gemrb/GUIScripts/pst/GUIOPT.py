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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUIOPT.py - scripts to control options windows mostly from GUIOPT winpack

# GUIOPT:
# 0 - Main options window (peacock tail)
# 1 - Video options window
# 2 - msg win with 1 button
# 3 - msg win with 2 buttons
# 4 - msg win with 3 buttons
# 5 - Audio options window
# 6 - Gameplay options window
# 8 - Feedback options window
# 9 - Autopause options window

###################################################
import Container
import GemRB
import GUICommon
import GUICommonWindows
import GUISAVE
import GUIOPTControls
from GUIDefines import *

###################################################
def InitOptionsWindow (Window):
	"""Open main options window (peacock tail)"""

	GemRB.GamePause (1, 1)

	Container.CloseContainerWindow ()

	def ConfigOptButton(button, strref, action):
		button.SetText (strref)
		button.OnPress (action)

	# Return to Game
	ConfigOptButton(Window.GetControl (0), 28638, Window.Close)

	# Quit Game
	ConfigOptButton(Window.GetControl (1), 2595, OpenQuitMsgWindow)

	# Load Game
	# german pst has two spaces that need to be squished
	LoadButton = Window.GetControl (2)
	LoadGameString = GemRB.GetString (2592)
	NewString = " ".join(LoadGameString.split())
	LoadButton.SetText (NewString)
	LoadButton.OnPress (OpenLoadMsgWindow)

	# Save Game
	ConfigOptButton(Window.GetControl (3), 20639, GUISAVE.OpenSaveWindow)

	# Video Options
	ConfigOptButton(Window.GetControl (4), 28781, OpenVideoOptionsWindow)

	# Audio Options
	ConfigOptButton(Window.GetControl (5), 29720, OpenAudioOptionsWindow)

	# Gameplay Options
	ConfigOptButton(Window.GetControl (6), 29722, OpenGameplayOptionsWindow)

	# Keyboard Mappings
	ConfigOptButton(Window.GetControl (7), 29723, OpenKeyboardMappingsWindow)

	# Movies
	ConfigOptButton(Window.GetControl (9), 38156, OpenMoviesWindow)

	# game version, e.g. v1.1.0000
	Label = Window.GetControl (0x10000007)
	Label.SetText (GemRB.Version)

	return

ToggleOptionsWindow = GUICommonWindows.CreateTopWinLoader(0, "GUIOPT", GUICommonWindows.ToggleWindow, InitOptionsWindow)
OpenOptionsWindow = GUICommonWindows.CreateTopWinLoader(0, "GUIOPT", GUICommonWindows.OpenWindowOnce, InitOptionsWindow)

def SaveSettings(Window, settings):
	# volume sliders are multiplers, so apply thay before commition
	volumes = ['Volume Ambients', 'Volume SFX',
				'Volume Voices', 'Volume Music',
				'Volume Movie'];

	for var in volumes:
		val = Window.GetVar(var)
		if val is not None:
			Window.SetVar(var, val * GemRB.GetVar(var))

	Window.StoreGlobalVariables(settings)
	GemRB.SaveConfig()
	Window.Close()

###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global VideoHelpText

	Window = GemRB.LoadWindow (1, "GUIOPT")
	
	settings = ['Brightness Correction', 'Gamma Correction',
		'SoftBlt', 'SoftMirrorBlt', 'SoftSrcKeyBlt'];
			
	Window.LoadGlobalVariables(settings)

	VideoHelpText = GUIOPTControls.OptHelpText ('VideoOptions', Window, 9, 31052)

	GUIOPTControls.OptDone (lambda: SaveSettings(Window, settings), Window, 7)
	GUIOPTControls.OptCancel (Window.Close, Window, 8)

	GUIOPTControls.OptSlider (31052, 31431, VideoHelpText, Window, 1, 10, 31234, "Brightness Correction", lambda: GammaFeedback(31431))
	GUIOPTControls.OptSlider (31052, 31459, VideoHelpText, Window, 2, 11, 31429, "Gamma Correction", lambda: GammaFeedback(31459))

	GUIOPTControls.OptCheckbox (31052, 31221, VideoHelpText, Window, 6, 15, 30898, "SoftBlt")
	GUIOPTControls.OptCheckbox (31052, 31216, VideoHelpText, Window, 4, 13, 30896, "SoftMirrorBlt")
	GUIOPTControls.OptCheckbox (31052, 31220, VideoHelpText, Window, 5, 14, 30897, "SoftSrcKeyBlt")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return
	
def GammaFeedback (feedback_ref):
	VideoHelpText.SetText (feedback_ref)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction")/5,GemRB.GetVar("Gamma Correction")/20)
	return

###################################################

saved_audio_options = {}

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global AudioHelpText

	Window = GemRB.LoadWindow (5, "GUIOPT")

	settings = ['Volume Ambients', 'Volume SFX',
		'Volume Voices', 'Volume Music', 'Volume Movie',
		'Environmental Audio', 'Sound Processing', 'Music Processing'];

	Window.LoadGlobalVariables(settings)

	AudioHelpText = GUIOPTControls.OptHelpText ('AudioOptions', Window, 9, 31210)

	GUIOPTControls.OptDone (lambda: SaveSettings(Window, settings), Window, 7)
	GUIOPTControls.OptCancel (Window.Close, Window, 8)

	GUIOPTControls.OptSlider (31210, 31227, AudioHelpText, Window, 1, 10, 31460, "Volume Ambients", lambda: UpdateVolume(31227))
	GUIOPTControls.OptSlider (31210, 31228, AudioHelpText, Window, 2, 11, 31466, "Volume SFX", lambda: UpdateVolume(31228))
	GUIOPTControls.OptSlider (31210, 31226, AudioHelpText, Window, 3, 12, 31467, "Volume Voices", lambda: UpdateVolume(31226))
	GUIOPTControls.OptSlider (31210, 31225, AudioHelpText, Window, 4, 13, 31468, "Volume Music", lambda: UpdateVolume(31225))
	GUIOPTControls.OptSlider (31210, 31229, AudioHelpText, Window, 5, 14, 31469, "Volume Movie", lambda: UpdateVolume(31229))
	
	GUIOPTControls.OptCheckbox (31210, 31224, AudioHelpText, Window, 6, 15, 30900, "Environmental Audio")
	GUIOPTControls.OptCheckbox (31210, 63244, AudioHelpText, Window, 16, 17, 63242, "Sound Processing")
	GUIOPTControls.OptCheckbox (31210, 63247, AudioHelpText, Window, 18, 19, 63243, "Music Processing")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateVolume (volume_ref):
	if AudioHelpText:
		AudioHelpText.SetText (volume_ref)
	GemRB.UpdateAmbientsVolume ()
	GemRB.UpdateMusicVolume ()

###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameplayHelpText

	Window = GemRB.LoadWindow (6, "GUIOPT")

	settings = ['Tooltips', 'Mouse Scroll Speed',
		'Keyboard Scroll Speed', 'Difficulty Level',
		'Always Dither', 'Gore', 'Always Run'];
			
	Window.LoadGlobalVariables(settings)

	GameplayHelpText = GUIOPTControls.OptHelpText ('GameplayOptions', Window, 12, 31212)

	GUIOPTControls.OptDone (lambda: SaveSettings(Window, settings), Window, 10)
	GUIOPTControls.OptCancel (Window.Close, Window, 11)

	GUIOPTControls.OptSlider (31212, 31232, GameplayHelpText, Window, 1, 13, 31481, "Tooltips", UpdateTooltips, TOOLTIP_DELAY_FACTOR)
	GUIOPTControls.OptSlider (31212, 31230, GameplayHelpText, Window, 2, 14, 31482, "Mouse Scroll Speed", UpdateMouseSpeed)
	GUIOPTControls.OptSlider (31212, 31231, GameplayHelpText, Window, 3, 15, 31480, "Keyboard Scroll Speed")
	GUIOPTControls.OptSlider (31212, 31233, GameplayHelpText, Window, 4, 16, 31479, "Difficulty Level")

	GUIOPTControls.OptCheckbox (31212, 31222, GameplayHelpText, Window, 5, 17, 31217, "Always Dither")
	GUIOPTControls.OptCheckbox (31212, 31223, GameplayHelpText, Window, 6, 18, 31218, "Gore")
	GUIOPTControls.OptCheckbox (31212, 62419, GameplayHelpText, Window, 22, 23, 62418, "Always Run", GUICommonWindows.ToggleAlwaysRun)

	PSTOptButton (31212, 31213, GameplayHelpText, Window, 8, 20, 31478, OpenFeedbackOptionsWindow)
	PSTOptButton (31212, 31214, GameplayHelpText, Window, 9, 21, 31470, OpenAutopauseOptionsWindow)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateTooltips ():
	GameplayHelpText.SetText (31232)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def UpdateMouseSpeed ():
	GameplayHelpText.SetText (31230)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

###################################################
	
def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global FeedbackHelpText

	Window = GemRB.LoadWindow (8, "GUIOPT")
	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)


	FeedbackHelpText = GUIOPTControls.OptHelpText ('FeedbackOptions', Window, 9, 37410)

	GUIOPTControls.OptDone (Window.Close, Window, 7)
	GUIOPTControls.OptCancel (Window.Close, Window, 8)

	GUIOPTControls.OptSlider (31213, 37411, FeedbackHelpText, Window, 1, 10, 37463, "Circle Feedback", UpdateMarkerFeedback)
	GUIOPTControls.OptSlider (31213, 37447, FeedbackHelpText, Window, 2, 11, 37586, "Locator Feedback Level")
	GUIOPTControls.OptSlider (31213, 54878, FeedbackHelpText, Window, 20, 21, 54879, "Selection Sounds Frequency")
	GUIOPTControls.OptSlider (31213, 54880, FeedbackHelpText, Window, 22, 23, 55012, "Command Sounds Frequency")

	# others have: to hit rolls, combat info, [actions], state changes, [selection text], misc
	# pst: states, misc, to hit rolls, combat info, [spell casting]; and separate sliders for selection and command
	# we harmonize it across games by likely breaking compatibility with the original
	# there's no need to disable values 4 and 16 (set by our defaults.ini), since their use takes the separate sliders into account
	GUIOPTControls.OptCheckbox (31213, 37460, FeedbackHelpText, Window, 6, 15, 37594, 'Effect Text Level', None, 8)
	GUIOPTControls.OptCheckbox (31213, 37462, FeedbackHelpText, Window, 17, 19, 37596, 'Effect Text Level', None, 32)
	GUIOPTControls.OptCheckbox (31213, 37453, FeedbackHelpText, Window, 3, 12, 37588, 'Effect Text Level', None, 1)
	GUIOPTControls.OptCheckbox (31213, 37457, FeedbackHelpText, Window, 4, 13, 37590, 'Effect Text Level', None, 2)
	GUIOPTControls.OptCheckbox (31213, 37458, FeedbackHelpText, Window, 5, 14, 37592, 'Effect Text Level', None, 64)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateMarkerFeedback ():
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global AutopauseHelpText
	
	Window = GemRB.LoadWindow (9, "GUIOPT")

	AutopauseHelpText = GUIOPTControls.OptHelpText ('AutopauseOptions', Window, 1, 31214)

	GUIOPTControls.OptDone (Window.Close, Window, 16)
	GUIOPTControls.OptCancel (Window.Close, Window, 17)

	# checkboxes OR the values if they associate to the same variable
	GUIOPTControls.OptCheckbox (31214, 37688, AutopauseHelpText, Window, 2, 9, 37598, "Auto Pause State", None, 4)
	GUIOPTControls.OptCheckbox (31214, 37689, AutopauseHelpText, Window, 3, 10, 37681, "Auto Pause State", None, 8)
	GUIOPTControls.OptCheckbox (31214, 37690, AutopauseHelpText, Window, 4, 11, 37682, "Auto Pause State", None, 16)
	GUIOPTControls.OptCheckbox (31214, 37691, AutopauseHelpText, Window, 5, 12, 37683, "Auto Pause State", None, 2)
	GUIOPTControls.OptCheckbox (31214, 37692, AutopauseHelpText, Window, 6, 13, 37684, "Auto Pause State", None, 1)
	GUIOPTControls.OptCheckbox (31214, 37693, AutopauseHelpText, Window, 7, 14, 37685, "Auto Pause State", None, 32)
	GUIOPTControls.OptCheckbox (31214, 37694, AutopauseHelpText, Window, 8, 15, 37686, "Auto Pause State", None, 64)

	Window.ShowModal (MODAL_SHADOW_GRAY)

###################################################
###################################################

def OpenLoadMsgWindow ():
	Window = GemRB.LoadWindow (3, "GUIOPT")

	# Load
	Button = Window.GetControl (0)
	Button.SetText (28648)
	Button.OnPress (LoadGame)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.OnPress (Window.Close)

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (39432)

	Window.ShowModal (MODAL_SHADOW_GRAY)


def LoadGame ():
	GemRB.QuitGame ()
	GemRB.SetNextScript ('GUILOAD')



###################################################

def OpenQuitMsgWindow ():
	Window = GemRB.LoadWindow (4, "GUIOPT")

	# Save
	Button = Window.GetControl (0)
	Button.SetText (28645)
	Button.OnPress (SaveGame)
	if GemRB.GetView("GC") is not None:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (2595)
	Button.OnPress (QuitGame)

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (4196)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	# The game has not been saved ....
	Text = Window.GetControl (3)
	Text.SetText (39430)  # or 39431 - cannot be saved atm

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitGame ():
	GemRB.QuitGame ()
	GemRB.SetNextScript ('Start')

def SaveGame ():
	GemRB.SetVar ("QuitAfterSave", 1)
	GUISAVE.OpenSaveWindow ()

###################################################

key_list = [
	('GemRB', None),
	('Grab pointer', '^G'),
	('Toggle fullscreen', '^F'),
	('Enable cheats', '^T'),
	('', None),
	
	('IE', None),
	('Open Inventory', 'I'),
	('Open Priest Spells', 'P'),
	('Open Mage Spells', 'S'),
	('Pause Game', 'SPC'),
	('Select Weapon', ''),
	('', None),
	]


KEYS_PAGE_SIZE = 60
KEYS_PAGE_COUNT = ((len (key_list) - 1) / KEYS_PAGE_SIZE)+ 1

def OpenKeyboardMappingsWindow ():
	global last_key_action

	last_key_action = None
		
	Window = GemRB.LoadWindow (0, "GUIKEYS")

	# Default
	Button = Window.GetControl (3)
	Button.SetText (49051)
	#Button.OnPress (None)

	# Done
	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.OnPress (Window.Close)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (5)
	Button.SetText (4196)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	keys_setup_page (Window, 0)

def keys_setup_page (Window, pageno):
	# Page n of n
	Label = Window.GetControl (0x10000001)
	GemRB.SetToken ('PAGE', str (pageno + 1))
	GemRB.SetToken ('NUMPAGES', str (KEYS_PAGE_COUNT))
	Label.SetText (49053)

	for i in range (KEYS_PAGE_SIZE):
		try:
			label, key = key_list[pageno * KEYS_PAGE_SIZE + i]
		except:
			label = ''
			key = None

		if key == None:
			# Section header
			Label = Window.GetControl (0x10000005 + i)
			Label.SetText ('')

			Label = Window.GetControl (0x10000041 + i)
			Label.SetText (label)
			Label.SetColor ({'r' : 0, 'g' : 255, 'b' : 255})
		else:
			Label = Window.GetControl (0x10000005 + i)
			Label.SetText (key)
			#Label.OnPress (lambda: OnActionLabelPress(Window))
			Label.SetVarAssoc ("KeyAction", i)	
			
			Label = Window.GetControl (0x10000041 + i)
			Label.SetText (label)
			#Label.OnPress (lambda: OnActionLabelPress(Window))
			Label.SetVarAssoc ("KeyAction", i)	


last_key_action = None
def OnActionLabelPress (Window):
	global last_key_action
	
	i = GemRB.GetVar ("KeyAction")

	if last_key_action != None:
		Label = Window.GetControl (0x10000005 + last_key_action)
		Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
		Label = Window.GetControl (0x10000041 + last_key_action)
		Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})
		
	Label = Window.GetControl (0x10000005 + i)
	Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})
	Label = Window.GetControl (0x10000041 + i)
	Label.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})

	last_key_action = i

	# 49155
	
###################################################

def OpenMoviesWindow ():
	Window = GemRB.LoadWindow (0, "GUIMOVIE")

	# Play Movie
	Button = Window.GetControl (2)
	Button.SetText (33034)
	Button.OnPress (OnPlayMoviePress)

	# Credits
	Button = Window.GetControl (3)
	Button.SetText (33078)
	Button.OnPress (OnCreditsPress)

	# Done
	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.OnPress (Window.Close)

	# movie list
	List = Window.GetControl (0)
	MovieTable = GemRB.LoadTable ("MOVIDESC")
	GemRB.SetVar ('SelectedMovie', 0)
	List.SetOptions([MovieTable.GetValue (i, 0) for i in range (MovieTable.GetRowCount ())], 'SelectedMovie', -1)

	Window.ShowModal (MODAL_SHADOW_BLACK)

###################################################
def OnPlayMoviePress ():
	selected = GemRB.GetVar ('SelectedMovie')
	
	MovieTable = GemRB.LoadTable ("MOVIDESC")
	key = MovieTable.GetRowName (selected)

	GemRB.PlayMovie (key, 1)

###################################################
def OnCreditsPress ():
	GemRB.PlayMovie ("CREDITS")

###################################################

def PSTOptButton (winname, ctlname, help_ta, window, button_id, label_id, label_strref, action):
	"""Standard subwindow button for option windows"""
	button = window.GetControl (button_id)
	button.OnPress (action)
	button.OnMouseEnter (lambda: help_ta.SetText (ctlname))
	button.OnMouseLeave (lambda: help_ta.SetText (winname))

	GUIOPTControls.OptBuddyLabel (window, label_id, label_strref, help_ta, ctlname, winname)

###################################################
# End of file GUIOPT.py

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
from ie_sounds import *
from GUIDefines import *

###################################################
def InitOptionsWindow (Window):
	"""Open main options window (peacock tail)"""

	GemRB.GamePause (1, 1)
	TrySavingConfiguration ()

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
	
def TrySavingConfiguration():
	if not GemRB.SaveConfig():
		print("ARGH, could not write config to disk!!")

###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""

	def OnClose(Window):
		TrySavingConfiguration()

	Window = GemRB.LoadWindow (1, "GUIOPT")
	Window.AddAlias ("SUB_WIN", 0)
	Window.OnClose (OnClose)

	GUIOPTControls.OptHelpText (9, 31052)
	GUIOPTControls.OptDone (Window.Close, 7)
	GUIOPTControls.OptCancel (Window.Close, 8)

	GUIOPTControls.OptSlider (31431, 1, 10, 31234, "Brightness Correction", lambda: GammaFeedback(31431))
	GUIOPTControls.OptSlider (31459, 2, 11, 31429, "Gamma Correction", lambda: GammaFeedback(31459))

	GUIOPTControls.OptCheckbox (31221, 6, 15, 30898, "SoftBlt")
	GUIOPTControls.OptCheckbox (31216, 4, 13, 30896, "SoftMirrorBlt")
	GUIOPTControls.OptCheckbox (31220, 5, 14, 30897, "SoftSrcKeyBlt")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return
	
def GammaFeedback (feedback_ref):
	GemRB.GetView ("OPTHELP").SetText (feedback_ref)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction")/5,GemRB.GetVar("Gamma Correction")/20)
	return

###################################################

saved_audio_options = {}

def OpenAudioOptionsWindow ():
	"""Open audio options window"""

	Window = GemRB.LoadWindow (5, "GUIOPT")
	Window.AddAlias ("SUB_WIN", 0)

	def OnClose(Window):
		# Restore values in case of cancel
		if GemRB.GetVar ("Cancel") == 1:
			for k, v in list(saved_audio_options.items ()):
				GemRB.SetVar (k, v)
			UpdateVolume (31210)

		TrySavingConfiguration()

	Window.OnClose (OnClose)

	# save values, so we can restore them on cancel
	for v in "Volume Ambients", "Volume SFX", "Volume Voices", "Volume Music", "Volume Movie", "Sound Processing", "Music Processing":
		saved_audio_options[v] = GemRB.GetVar (v)

	GUIOPTControls.OptHelpText (9, 31210)
	GUIOPTControls.OptDone (Window.Close, 7)
	GUIOPTControls.OptCancel (Window.Close, 8)

	GUIOPTControls.OptSlider (31227, 1, 10, 31460, "Volume Ambients", lambda: UpdateVolume(31227))
	GUIOPTControls.OptSlider (31228, 2, 11, 31466, "Volume SFX", lambda: UpdateVolume(31228))
	GUIOPTControls.OptSlider (31226, 3, 12, 31467, "Volume Voices", lambda: UpdateVolume(31226))
	GUIOPTControls.OptSlider (31225, 4, 13, 31468, "Volume Music", lambda: UpdateVolume(31225))
	GUIOPTControls.OptSlider (31229, 5, 14, 31469, "Volume Movie", lambda: UpdateVolume(31229))
	
	GUIOPTControls.OptCheckbox (31224, 6, 15, 30900, "Environmental Audio")
	GUIOPTControls.OptCheckbox (63244, 16, 17, 63242, "Sound Processing")
	GUIOPTControls.OptCheckbox (63247, 18, 19, 63243, "Music Processing")

	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateVolume (volume_ref):
	helpTA = GemRB.GetView ("OPTHELP")
	if helpTA:
		helpTA.SetText (volume_ref)
	GemRB.UpdateVolume (GEM_SND_VOL_MUSIC)
	GemRB.UpdateVolume (GEM_SND_VOL_AMBIENTS)

###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""

	Window = GemRB.LoadWindow (6, "GUIOPT")
	Window.AddAlias ("SUB_WIN", 0)

	def OnClose(Window):
		TrySavingConfiguration()

	Window.OnClose (OnClose)

	GUIOPTControls.OptHelpText (12, 31212)

	GUIOPTControls.OptDone (Window.Close, 10)
	GUIOPTControls.OptCancel (Window.Close, 11)

	GUIOPTControls.OptSlider (31232, 1, 13, 31481, "Tooltips", UpdateTooltips, 1)
	GUIOPTControls.OptSlider (31230, 2, 14, 31482, "Mouse Scroll Speed", UpdateMouseSpeed)
	GUIOPTControls.OptSlider (31231, 3, 15, 31480, "Keyboard Scroll Speed")
	GUIOPTControls.OptSlider (31233, 4, 16, 31479, "Difficulty Level")

	GUIOPTControls.OptCheckbox (31222, 5, 17, 31217, "Always Dither")
	GUIOPTControls.OptCheckbox (31223, 6, 18, 31218, "Gore")
	GUIOPTControls.OptCheckbox (62419, 22, 23, 62418, "Always Run", GUICommonWindows.ToggleAlwaysRun)

	PSTOptButton (31213, 8, 20, 31478, OpenFeedbackOptionsWindow)
	PSTOptButton (31214, 9, 21, 31470, OpenAutopauseOptionsWindow)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def UpdateTooltips ():
	GemRB.GetView ("OPTHELP").SetText (31232)
	GemRB.UpdateTooltipDelay()

def UpdateMouseSpeed ():
	GemRB.GetView ("OPTHELP").SetText (31230)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

###################################################
	
def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""

	Window = GemRB.LoadWindow (8, "GUIOPT")
	Window.AddAlias ("SUB_WIN", 1)
	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)

	GUIOPTControls.OptHelpText (9, 31213)
	GUIOPTControls.OptDone (Window.Close, 7)
	GUIOPTControls.OptCancel (Window.Close, 8)

	GUIOPTControls.OptSlider (37411, 1, 10, 37463, "Circle Feedback", UpdateMarkerFeedback)
	GUIOPTControls.OptSlider (37447, 2, 11, 37586, "Locator Feedback Level")
	GUIOPTControls.OptSlider (54878, 20, 21, 54879, "Selection Sounds Frequency")
	GUIOPTControls.OptSlider (54880, 22, 23, 55012, "Command Sounds Frequency")

	# others have: to hit rolls, combat info, [actions], state changes, [selection text], misc
	# pst: states, misc, to hit rolls, combat info, [spell casting]; and separate sliders for selection and command
	# we harmonize it across games by likely breaking compatibility with the original
	# there's no need to disable values 4 and 16 (set by our defaults.ini), since their use takes the separate sliders into account
	GUIOPTControls.OptCheckbox (37460, 6, 15, 37594, 'Effect Text Level', None, 8)
	GUIOPTControls.OptCheckbox (37462, 17, 19, 37596, 'Effect Text Level', None, 32)
	GUIOPTControls.OptCheckbox (37453, 3, 12, 37588, 'Effect Text Level', None, 1)
	GUIOPTControls.OptCheckbox (37457, 4, 13, 37590, 'Effect Text Level', None, 2)
	GUIOPTControls.OptCheckbox (37458, 5, 14, 37592, 'Effect Text Level', None, 64)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateMarkerFeedback ():
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	
	Window = GemRB.LoadWindow (9, "GUIOPT")
	Window.AddAlias ("SUB_WIN", 1)

	GUIOPTControls.OptHelpText (1, 31214)
	GUIOPTControls.OptDone (Window.Close, 16)
	GUIOPTControls.OptCancel (Window.Close, 17)

	# checkboxes OR the values if they associate to the same variable
	GUIOPTControls.OptCheckbox (37688, 2, 9, 37598, "Auto Pause State", None, 4)
	GUIOPTControls.OptCheckbox (37689, 3, 10, 37681, "Auto Pause State", None, 8)
	GUIOPTControls.OptCheckbox (37690, 4, 11, 37682, "Auto Pause State", None, 16)
	GUIOPTControls.OptCheckbox (37691, 5, 12, 37683, "Auto Pause State", None, 2)
	GUIOPTControls.OptCheckbox (37692, 6, 13, 37684, "Auto Pause State", None, 1)
	GUIOPTControls.OptCheckbox (37693, 7, 14, 37685, "Auto Pause State", None, 32)
	GUIOPTControls.OptCheckbox (37694, 8, 15, 37686, "Auto Pause State", None, 64)

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
KEYS_PAGE_COUNT = ((len (key_list) - 1) // KEYS_PAGE_SIZE) + 1

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

def PSTOptButton (ctlname, button_id, label_id, label_strref, action):
	"""Standard subwindow button for option windows"""

	window = GemRB.GetView ("SUB_WIN")
	button = window.GetControl (button_id)
	button.OnPress (action)
	help_ta = GemRB.GetView ("OPTHELP")
	button.OnMouseEnter (lambda: help_ta.SetText (ctlname))
	button.OnMouseLeave (lambda: help_ta.SetText (help_ta.Value))

	GUIOPTControls.OptBuddyLabel (label_id, label_strref, ctlname, help_ta.Value)

###################################################
# End of file GUIOPT.py

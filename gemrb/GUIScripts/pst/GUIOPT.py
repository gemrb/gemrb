# -*-python-*-
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
import CommonWindow
import GemRB
import GUICommon
import GUICommonWindows
import GUISAVE
import GUIOPTControls
from GUIDefines import *

###################################################
OptionsWindow = None
SubOptionsWindow = None
SubSubOptionsWindow = None
LoadMsgWindow = None
QuitMsgWindow = None

###################################################
def OpenOptionsWindow ():
	"""Open main options window (peacock tail)"""
	global OptionsWindow

	if GUICommon.CloseOtherWindow (OpenOptionsWindow):
		GemRB.HideGUI ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		GemRB.SetVar ("OtherWindow", -1)
		GUICommonWindows.EnableAnimatedWindows ()
		OptionsWindow = None
		
		GemRB.UnhideGUI ()
		return

	GemRB.GamePause (1, 1)

	CommonWindow.CloseContainerWindow ()
	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIOPT")
	OptionsWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", OptionsWindow.ID)
	GUICommonWindows.DisableAnimatedWindows ()
	
	# Return to Game
	Button = Window.GetControl (0)
	Button.SetText (28638)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenOptionsWindow)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (2595)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuitMsgWindow)

	# Load Game
	Button = Window.GetControl (2)
	Button.SetText (2592)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLoadMsgWindow)

	# Save Game
	Button = Window.GetControl (3)
	Button.SetText (20639)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUISAVE.OpenSaveWindow)

	# Video Options
	Button = Window.GetControl (4)
	Button.SetText (28781)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenVideoOptionsWindow)

	# Audio Options
	Button = Window.GetControl (5)
	Button.SetText (29720)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenAudioOptionsWindow)

	# Gameplay Options
	Button = Window.GetControl (6)
	Button.SetText (29722)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenGameplayOptionsWindow)

	# Keyboard Mappings
	Button = Window.GetControl (7)
	Button.SetText (29723)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenKeyboardMappingsWindow)

	# Movies
	Button = Window.GetControl (9)
	Button.SetText (38156)   # or  2594
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMoviesWindow)

	# game version, e.g. v1.1.0000
	Label = Window.GetControl (0x10000007)
	Label.SetText (GEMRB_VERSION)
	
	GemRB.UnhideGUI ()

def TrySavingConfiguration():
	if not GemRB.SaveConfig():
		print "ARGH, could not write config to disk!!"

###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global SubOptionsWindow, VideoHelpText

	GemRB.HideGUI ()

	if SubOptionsWindow:
		CloseSubOptionsWindow ()
		TrySavingConfiguration()
		GemRB.UnhideGUI ()
		return

	SubOptionsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar ("FloatWindow", SubOptionsWindow.ID)


	VideoHelpText = GUIOPTControls.OptHelpText ('VideoOptions', Window, 9, 31052)

	GUIOPTControls.OptDone (OpenVideoOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenVideoOptionsWindow, Window, 8)

	GUIOPTControls.OptSlider (31052, 31431, VideoHelpText, Window, 1, 10, 31234, "Brightness Correction", lambda: GammaFeedback(31431))
	GUIOPTControls.OptSlider (31052, 31459, VideoHelpText, Window, 2, 11, 31429, "Gamma Correction", lambda: GammaFeedback(31459))

	GUIOPTControls.OptCheckbox (31052, 31221, VideoHelpText, Window, 6, 15, 30898, "SoftBlt")
	GUIOPTControls.OptCheckbox (31052, 31216, VideoHelpText, Window, 4, 13, 30896, "SoftMirrorBlt")
	GUIOPTControls.OptCheckbox (31052, 31220, VideoHelpText, Window, 5, 14, 30897, "SoftSrcKeyBlt")

	GemRB.UnhideGUI ()
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
	global SubOptionsWindow, AudioHelpText

	GemRB.HideGUI ()

	if SubOptionsWindow:
		CloseSubOptionsWindow ()

		# Restore values in case of cancel
		if GemRB.GetVar ("Cancel") == 1:
			for k, v in saved_audio_options.items ():
				GemRB.SetVar (k, v)
			AudioHelpText = None
			UpdateVolume (31210)

		TrySavingConfiguration()
		GemRB.UnhideGUI ()
		return

	SubOptionsWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", SubOptionsWindow.ID)
	

	# save values, so we can restore them on cancel
	for v in "Volume Ambients", "Volume SFX", "Volume Voices", "Volume Music", "Volume Movie", "Sound Processing", "Music Processing":
		saved_audio_options[v] = GemRB.GetVar (v)


	AudioHelpText = GUIOPTControls.OptHelpText ('AudioOptions', Window, 9, 31210)

	GUIOPTControls.OptDone (OpenAudioOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenAudioOptionsWindow, Window, 8)

	GUIOPTControls.OptSlider (31210, 31227, AudioHelpText, Window, 1, 10, 31460, "Volume Ambients", lambda: UpdateVolume(31227))
	GUIOPTControls.OptSlider (31210, 31228, AudioHelpText, Window, 2, 11, 31466, "Volume SFX", lambda: UpdateVolume(31228))
	GUIOPTControls.OptSlider (31210, 31226, AudioHelpText, Window, 3, 12, 31467, "Volume Voices", lambda: UpdateVolume(31226))
	GUIOPTControls.OptSlider (31210, 31225, AudioHelpText, Window, 4, 13, 31468, "Volume Music", lambda: UpdateVolume(31225))
	GUIOPTControls.OptSlider (31210, 31229, AudioHelpText, Window, 5, 14, 31469, "Volume Movie", lambda: UpdateVolume(31229))
	
	GUIOPTControls.OptCheckbox (31210, 31224, AudioHelpText, Window, 6, 15, 30900, "Environmental Audio")
	GUIOPTControls.OptCheckbox (31210, 63244, AudioHelpText, Window, 16, 17, 63242, "Sound Processing")
	GUIOPTControls.OptCheckbox (31210, 63247, AudioHelpText, Window, 18, 19, 63243, "Music Processing")

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateVolume (volume_ref):
	if AudioHelpText:
		AudioHelpText.SetText (volume_ref)
	GemRB.UpdateAmbientsVolume ()
	GemRB.UpdateMusicVolume ()

###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global SubOptionsWindow, GameplayHelpText

	GemRB.HideGUI ()

	if SubOptionsWindow:
		CloseSubOptionsWindow ()
		TrySavingConfiguration()
		GemRB.UnhideGUI ()
		return

	SubOptionsWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("FloatWindow", SubOptionsWindow.ID)
	

	GameplayHelpText = GUIOPTControls.OptHelpText ('GameplayOptions', Window, 12, 31212)

	GUIOPTControls.OptDone (OpenGameplayOptionsWindow, Window, 10)
	GUIOPTControls.OptCancel (OpenGameplayOptionsWindow, Window, 11)

	GUIOPTControls.OptSlider (31212, 31232, GameplayHelpText, Window, 1, 13, 31481, "Tooltips", UpdateTooltips, TOOLTIP_DELAY_FACTOR)
	GUIOPTControls.OptSlider (31212, 31230, GameplayHelpText, Window, 2, 14, 31482, "Mouse Scroll Speed", UpdateMouseSpeed)
	GUIOPTControls.OptSlider (31212, 31231, GameplayHelpText, Window, 3, 15, 31480, "Keyboard Scroll Speed")
	GUIOPTControls.OptSlider (31212, 31233, GameplayHelpText, Window, 4, 16, 31479, "Difficulty Level")

	GUIOPTControls.OptCheckbox (31212, 31222, GameplayHelpText, Window, 5, 17, 31217, "Always Dither")
	GUIOPTControls.OptCheckbox (31212, 31223, GameplayHelpText, Window, 6, 18, 31218, "Gore")
	GUIOPTControls.OptCheckbox (31212, 62419, GameplayHelpText, Window, 22, 23, 62418, "Always Run")

	PSTOptButton (31212, 31213, GameplayHelpText, Window, 8, 20, 31478, OpenFeedbackOptionsWindow)
	PSTOptButton (31212, 31214, GameplayHelpText, Window, 9, 21, 31470, OpenAutopauseOptionsWindow)

	GemRB.UnhideGUI ()
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
	global SubSubOptionsWindow, FeedbackHelpText
	
	GemRB.HideGUI ()

	if SubSubOptionsWindow:
		CloseSubSubOptionsWindow ()
		GemRB.UnhideGUI ()
		return

	SubSubOptionsWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("FloatWindow", SubSubOptionsWindow.ID)
	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)


	FeedbackHelpText = GUIOPTControls.OptHelpText ('FeedbackOptions', Window, 9, 37410)

	GUIOPTControls.OptDone (OpenFeedbackOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenFeedbackOptionsWindow, Window, 8)

	GUIOPTControls.OptSlider (31213, 37411, FeedbackHelpText, Window, 1, 10, 37463, "Circle Feedback", UpdateMarkerFeedback)
	GUIOPTControls.OptSlider (31213, 37447, FeedbackHelpText, Window, 2, 11, 37586, "Locator Feedback Level")
	GUIOPTControls.OptSlider (31213, 54878, FeedbackHelpText, Window, 20, 21, 54879, "Selection Sounds Frequency")
	GUIOPTControls.OptSlider (31213, 54880, FeedbackHelpText, Window, 22, 23, 55012, "Command Sounds Frequency")

	# TODO: once the pst overhead messaging system is in place, add the relevant game vars below
	GUIOPTControls.OptCheckbox (31213, 37460, FeedbackHelpText, Window, 6, 15, 37594, "")
	GUIOPTControls.OptCheckbox (31213, 37462, FeedbackHelpText, Window, 17, 19, 37596, "")
	GUIOPTControls.OptCheckbox (31213, 37453, FeedbackHelpText, Window, 3, 12, 37588, "")
	GUIOPTControls.OptCheckbox (31213, 37457, FeedbackHelpText, Window, 4, 13, 37590, "")
	GUIOPTControls.OptCheckbox (31213, 37458, FeedbackHelpText, Window, 5, 14, 37592, "")

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateMarkerFeedback ():
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global SubSubOptionsWindow, AutopauseHelpText
	
	GemRB.HideGUI ()

	if SubSubOptionsWindow:
		CloseSubSubOptionsWindow ()
		GemRB.UnhideGUI ()
		return

	
	SubSubOptionsWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("FloatWindow", SubSubOptionsWindow.ID)


	AutopauseHelpText = GUIOPTControls.OptHelpText ('AutopauseOptions', Window, 1, 31214)

	GUIOPTControls.OptDone (OpenAutopauseOptionsWindow, Window, 16)
	GUIOPTControls.OptCancel (OpenAutopauseOptionsWindow, Window, 17)

	# checkboxes OR the values if they associate to the same variable
	GUIOPTControls.OptCheckbox (31214, 37688, AutopauseHelpText, Window, 2, 9, 37598, "Auto Pause State", None, 4)
	GUIOPTControls.OptCheckbox (31214, 37689, AutopauseHelpText, Window, 3, 10, 37681, "Auto Pause State", None, 8)
	GUIOPTControls.OptCheckbox (31214, 37690, AutopauseHelpText, Window, 4, 11, 37682, "Auto Pause State", None, 16)
	GUIOPTControls.OptCheckbox (31214, 37691, AutopauseHelpText, Window, 5, 12, 37683, "Auto Pause State", None, 2)
	GUIOPTControls.OptCheckbox (31214, 37692, AutopauseHelpText, Window, 6, 13, 37684, "Auto Pause State", None, 1)
	GUIOPTControls.OptCheckbox (31214, 37693, AutopauseHelpText, Window, 7, 14, 37685, "Auto Pause State", None, 32)
	GUIOPTControls.OptCheckbox (31214, 37694, AutopauseHelpText, Window, 8, 15, 37686, "Auto Pause State", None, 64)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)

###################################################
###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.HideGUI()

	if LoadMsgWindow:		
		if LoadMsgWindow:
			LoadMsgWindow.Unload ()
		LoadMsgWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	LoadMsgWindow = Window = GemRB.LoadWindow (3)
	GemRB.SetVar ("FloatWindow", LoadMsgWindow.ID)
	
	# Load
	Button = Window.GetControl (0)
	Button.SetText (28648)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadGame)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLoadMsgWindow)

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (39432)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)


def LoadGame ():
	OpenLoadMsgWindow () ##close self
	OpenOptionsWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ('GUILOAD')



###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	#GemRB.HideGUI()

	if QuitMsgWindow:		
		if QuitMsgWindow:
			QuitMsgWindow.Unload ()
		QuitMsgWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		#GemRB.UnhideGUI ()
		return
		
	QuitMsgWindow = Window = GemRB.LoadWindow (4)
	GemRB.SetVar ("FloatWindow", QuitMsgWindow.ID)
	
	# Save
	Button = Window.GetControl (0)
	Button.SetText (28645)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SaveGame)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (2595)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGame)

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuitMsgWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# The game has not been saved ....
	Text = Window.GetControl (3)
	Text.SetText (39430)  # or 39431 - cannot be saved atm

	#GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitGame ():
	OpenOptionsWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ('Start')

def SaveGame ():
	GemRB.SetVar ("QuitAfterSave", 1)
	OpenOptionsWindow ()
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
	global SubOptionsWindow
	global last_key_action

	last_key_action = None

	GemRB.HideGUI()

	if SubOptionsWindow:
		CloseSubOptionsWindow ()
		GemRB.SetVar ("OtherWindow", OptionsWindow.ID)
		
		GemRB.LoadWindowPack ("GUIOPT")
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIKEYS")
	SubOptionsWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", SubOptionsWindow.ID)

	# Default
	Button = Window.GetControl (3)
	Button.SetText (49051)
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)

	# Done
	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenKeyboardMappingsWindow)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (5)
	Button.SetText (4196)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenKeyboardMappingsWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	keys_setup_page (0)

	GemRB.UnhideGUI ()

def keys_setup_page (pageno):
	Window = SubOptionsWindow

	# Page n of n
	Label = Window.GetControl (0x10000001)
	#txt = GemRB.ReplaceVarsInText (49053, {'PAGE': str (pageno + 1), 'NUMPAGES': str (KEYS_PAGE_COUNT)})
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
			Label.SetTextColor (0, 255, 255)

		else:
			Label = Window.GetControl (0x10000005 + i)
			Label.SetText (key)
			Label.SetEvent (IE_GUI_LABEL_ON_PRESS, OnActionLabelPress)
			Label.SetVarAssoc ("KeyAction", i)	
			
			Label = Window.GetControl (0x10000041 + i)
			Label.SetText (label)
			Label.SetEvent (IE_GUI_LABEL_ON_PRESS, OnActionLabelPress)
			Label.SetVarAssoc ("KeyAction", i)	


last_key_action = None
def OnActionLabelPress ():
	global last_key_action
	
	Window = SubOptionsWindow
	i = GemRB.GetVar ("KeyAction")

	if last_key_action != None:
		Label = Window.GetControl (0x10000005 + last_key_action)
		Label.SetTextColor (255, 255, 255)
		Label = Window.GetControl (0x10000041 + last_key_action)
		Label.SetTextColor (255, 255, 255)
		
	Label = Window.GetControl (0x10000005 + i)
	Label.SetTextColor (255, 255, 0)
	Label = Window.GetControl (0x10000041 + i)
	Label.SetTextColor (255, 255, 0)

	last_key_action = i

	# 49155
	
###################################################

def OpenMoviesWindow ():
	global SubOptionsWindow

	GemRB.HideGUI()

	if SubOptionsWindow:
		CloseSubOptionsWindow ()
		GemRB.LoadWindowPack ("GUIOPT")
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIMOVIE")
	SubOptionsWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("FloatWindow", SubOptionsWindow.ID)

	# Play Movie
	Button = Window.GetControl (2)
	Button.SetText (33034)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnPlayMoviePress)

	# Credits
	Button = Window.GetControl (3)
	Button.SetText (33078)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnCreditsPress)

	# Done
	Button = Window.GetControl (4)
	Button.SetText (1403)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMoviesWindow)

	# movie list
	List = Window.GetControl (0)
	List.SetVarAssoc ('SelectedMovie', -1)

	MovieTable = GemRB.LoadTable ("MOVIDESC")
	List.SetOptions([MovieTable.GetValue (i, 0) for i in range (MovieTable.GetRowCount ())])

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_BLACK)

###################################################
def OnPlayMoviePress ():
	selected = GemRB.GetVar ('SelectedMovie')

	# FIXME: This should not happen, when the PlayMovie button gets
	#   properly disabled/enabled, but it does not now
	if selected == -1:
		return
	
	MovieTable = GemRB.LoadTable ("MOVIDESC")
	key = MovieTable.GetRowName (selected)

	GemRB.PlayMovie (key, 1)

###################################################
def OnCreditsPress ():
	GemRB.PlayMovie ("CREDITS")

###################################################

def CloseSubOptionsWindow ():
	global SubOptionsWindow

	if SubOptionsWindow:
		SubOptionsWindow.Unload ()
		SubOptionsWindow = None
	GemRB.SetVar ("FloatWindow", -1)
	return

def CloseSubSubOptionsWindow ():
	global SubSubOptionsWindow, SubOptionsWindow

	if SubSubOptionsWindow:
		SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None
	if SubOptionsWindow:
		SubOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
		GemRB.SetVar ("FloatWindow", SubOptionsWindow.ID)
	return

def PSTOptButton (winname, ctlname, help_ta, window, button_id, label_id, label_strref, action):
	"""Standard subwindow button for option windows"""
	button = window.GetControl (button_id)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, action)
	button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, lambda: help_ta.SetText (ctlname))
	button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, lambda: help_ta.SetText (winname))

	GUIOPTControls.OptBuddyLabel (window, label_id, label_strref, help_ta, ctlname, winname)

###################################################
# End of file GUIOPT.py

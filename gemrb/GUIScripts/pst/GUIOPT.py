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
import GemRB
import GUICommon
import GUICommonWindows
import GUISAVE
import GUIOPTControls
from GUIDefines import *

###################################################
OptionsWindow = None
VideoOptionsWindow = None
AudioOptionsWindow = None
GameplayOptionsWindow = None
FeedbackOptionsWindow = None
AutopauseOptionsWindow = None
LoadMsgWindow = None
QuitMsgWindow = None
MoviesWindow = None
KeysWindow = None

###################################################
def OpenOptionsWindow ():
	"""Open main options window (peacock tail)"""
	global OptionsWindow

	if GUICommon.CloseOtherWindow (OpenOptionsWindow):
		if VideoOptionsWindow: OpenVideoOptionsWindow ()
		if AudioOptionsWindow: OpenAudioOptionsWindow ()
		if GameplayOptionsWindow: OpenGameplayOptionsWindow ()
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow ()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow ()
		if LoadMsgWindow: OpenLoadMsgWindow ()
		if QuitMsgWindow: OpenQuitMsgWindow ()
		if KeysWindow: OpenKeysWindow ()
		if MoviesWindow: OpenMoviesWindow ()
		
		GemRB.HideGUI ()
		if OptionsWindow:
			OptionsWindow.Unload ()
		GemRB.SetVar ("OtherWindow", -1)
		GUICommonWindows.EnableAnimatedWindows ()
		OptionsWindow = None
		
		GemRB.UnhideGUI ()
		return
		
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
	
	#Window.SetVisible (WINDOW_VISIBLE)
	GemRB.UnhideGUI ()


	
###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global VideoOptionsWindow, VideoHelpText

	GemRB.HideGUI ()

	if VideoOptionsWindow:
		if VideoOptionsWindow:
			VideoOptionsWindow.Unload ()
		VideoOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return

	
	VideoOptionsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar ("FloatWindow", VideoOptionsWindow.ID)


	VideoHelpText = GUIOPTControls.OptHelpText ('VideoOptions', Window, 9, 31052)

	GUIOPTControls.OptDone (OpenVideoOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenVideoOptionsWindow, Window, 8)

	PSTOptSlider ('VideoOptions', 'Brightness', Window, 1, 10, 31234, "Brightness Correction", GammaFeedback, 1)
	PSTOptSlider ('VideoOptions', 'Contrast', Window, 2, 11, 31429, "Gamma Correction", GammaFeedback, 1)

	PSTOptCheckbox ('VideoOptions', 'SoftwareBlitting', Window, 6, 15, 30898, None) #TODO: SoftBlt
	PSTOptCheckbox ('VideoOptions', 'SoftwareMirroring', Window, 4, 13, 30896, None) #TODO: SoftMirrorBlt
	PSTOptCheckbox ('VideoOptions', 'SoftwareTransparency', Window, 5, 14, 30897, None) #TODO: SoftSrcKeyBlt

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return
	
def GammaFeedback ():
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction")/5,GemRB.GetVar("Gamma Correction")/5)
	return

def DisplayHelpVideoOptions ():
	VideoHelpText.SetText (31052)

def DisplayHelpBrightness ():
	VideoHelpText.SetText (31431)

def DisplayHelpContrast ():
	VideoHelpText.SetText (31459)

def DisplayHelpSoftwareBlitting ():
	VideoHelpText.SetText (31221)

def DisplayHelpSoftwareMirroring ():
	VideoHelpText.SetText (31216)

def DisplayHelpSoftwareTransparency ():
	VideoHelpText.SetText (31220)



###################################################

saved_audio_options = {}

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global AudioOptionsWindow, AudioHelpText

	GemRB.HideGUI ()

	if AudioOptionsWindow:
		if AudioOptionsWindow:
			AudioOptionsWindow.Unload ()
		AudioOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		# Restore values in case of cancel
		if GemRB.GetVar ("Cancel") == 1:
			for k, v in saved_audio_options.items ():
				GemRB.SetVar (k, v)
			UpdateVolume ()
		
		GemRB.UnhideGUI ()
		return

	
	AudioOptionsWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("FloatWindow", AudioOptionsWindow.ID)
	

	# save values, so we can restore them on cancel
	for v in "Volume Ambients", "Volume SFX", "Volume Voices", "Volume Music", "Volume Movie", "Sound Processing", "Music Processing":
		saved_audio_options[v] = GemRB.GetVar (v)


	AudioHelpText = GUIOPTControls.OptHelpText ('AudioOptions', Window, 9, 31210)

	GUIOPTControls.OptDone (OpenAudioOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenAudioOptionsWindow, Window, 8)

	PSTOptSlider ('AudioOptions', 'AmbientVolume', Window, 1, 10, 31460, "Volume Ambients", UpdateVolume)
	PSTOptSlider ('AudioOptions', 'SoundFXVolume', Window, 2, 11, 31466, "Volume SFX", UpdateVolume)
	PSTOptSlider ('AudioOptions', 'VoiceVolume', Window, 3, 12, 31467, "Volume Voices", UpdateVolume)
	PSTOptSlider ('AudioOptions', 'MusicVolume', Window, 4, 13, 31468, "Volume Music", UpdateVolume)
	PSTOptSlider ('AudioOptions', 'MovieVolume', Window, 5, 14, 31469, "Volume Movie", UpdateVolume)
	
	PSTOptCheckbox ('AudioOptions', 'CreativeEAX', Window, 6, 15, 30900, "Environmental Audio")
	PSTOptCheckbox ('AudioOptions', 'SoundProcessing', Window, 16, 17, 63242, "Sound Processing")
	PSTOptCheckbox ('AudioOptions', 'MusicProcessing', Window, 18, 19, 63243, "Music Processing")

	#AudioOptionsWindow.SetVisible (WINDOW_VISIBLE)
	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateVolume ():
	GemRB.UpdateAmbientsVolume ()
	GemRB.UpdateMusicVolume ()
	
def DisplayHelpAudioOptions ():
	AudioHelpText.SetText (31210)

def DisplayHelpAmbientVolume ():
	AudioHelpText.SetText (31227)
	
def DisplayHelpSoundFXVolume ():
	AudioHelpText.SetText (31228)

def DisplayHelpVoiceVolume ():
	AudioHelpText.SetText (31226)

def DisplayHelpMusicVolume ():
	AudioHelpText.SetText (31225)

def DisplayHelpMovieVolume ():
	AudioHelpText.SetText (31229)

def DisplayHelpCreativeEAX ():
	AudioHelpText.SetText (31224)

def DisplayHelpSoundProcessing ():
	AudioHelpText.SetText (63244)
	
def DisplayHelpMusicProcessing ():
	AudioHelpText.SetText (63247)



###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameplayOptionsWindow, GameplayHelpText

	GemRB.HideGUI ()

	if GameplayOptionsWindow:
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow()

		if GameplayOptionsWindow:
			GameplayOptionsWindow.Unload ()
		GameplayOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)
		
		GemRB.UnhideGUI ()
		return

	
	GameplayOptionsWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("FloatWindow", GameplayOptionsWindow.ID)
	

	GameplayHelpText = GUIOPTControls.OptHelpText ('GameplayOptions', Window, 12, 31212)

	GUIOPTControls.OptDone (OpenGameplayOptionsWindow, Window, 10)
	GUIOPTControls.OptCancel (OpenGameplayOptionsWindow, Window, 11)

	PSTOptSlider ('GameplayOptions', 'TooltipDelay', Window, 1, 13, 31481, "Tooltips", UpdateTooltips, TOOLTIP_DELAY_FACTOR)
	PSTOptSlider ('GameplayOptions', 'MouseScrollingSpeed', Window, 2, 14, 31482, "Mouse Scroll Speed", UpdateMouseSpeed)
	PSTOptSlider ('GameplayOptions', 'KeyboardScrollingSpeed', Window, 3, 15, 31480, "Keyboard Scroll Speed", UpdateKeyboardSpeed)
	PSTOptSlider ('GameplayOptions', 'Difficulty', Window, 4, 16, 31479, "Difficulty Level")

	PSTOptCheckbox ('GameplayOptions', 'DitherAlways', Window, 5, 17, 31217, "Always Dither")
	PSTOptCheckbox ('GameplayOptions', 'Gore', Window, 6, 18, 31218, "Gore???")
	PSTOptCheckbox ('GameplayOptions', 'AlwaysRun', Window, 22, 23, 62418, "Always Run")

	PSTOptButton ('GameplayOptions', 'FeedbackOptions', Window, 8, 20, 31478)
	PSTOptButton ('GameplayOptions', 'AutopauseOptions', Window, 9, 21, 31470)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpGameplayOptions ():
	GameplayHelpText.SetText (31212)

def UpdateTooltips ():
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpTooltipDelay ():
	GameplayHelpText.SetText (31232)

def UpdateMouseSpeed ():
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

def DisplayHelpMouseScrollingSpeed ():
	GameplayHelpText.SetText (31230)

def UpdateKeyboardSpeed ():
	#GemRB.SetKeyboardScrollSpeed (GemRB.GetVar ("Keyboard Scroll Speed") )
	return

def DisplayHelpKeyboardScrollingSpeed ():
	GameplayHelpText.SetText (31231)

def DisplayHelpDifficulty ():
	GameplayHelpText.SetText (31233)

def DisplayHelpDitherAlways ():
	GameplayHelpText.SetText (31222)

def DisplayHelpGore ():
	GameplayHelpText.SetText (31223)

def DisplayHelpAlwaysRun ():
	GameplayHelpText.SetText (62419)

def DisplayHelpFeedbackOptions ():
	GameplayHelpText.SetText (31213)

def DisplayHelpAutopauseOptions ():
	GameplayHelpText.SetText (31214)



###################################################
	
def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global FeedbackOptionsWindow, FeedbackHelpText
	
	GemRB.HideGUI ()

	if FeedbackOptionsWindow:
		if FeedbackOptionsWindow:
			FeedbackOptionsWindow.Unload ()
		FeedbackOptionsWindow = None
		GemRB.SetVar ("FloatWindow", GameplayOptionsWindow.ID)
		
		GemRB.UnhideGUI ()
		GameplayOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	
	FeedbackOptionsWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("FloatWindow", FeedbackOptionsWindow.ID)
	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)


	FeedbackHelpText = GUIOPTControls.OptHelpText ('FeedbackOptions', Window, 9, 37410)

	GUIOPTControls.OptDone (OpenFeedbackOptionsWindow, Window, 7)
	GUIOPTControls.OptCancel (OpenFeedbackOptionsWindow, Window, 8)

	PSTOptSlider ('FeedbackOptions', 'MarkerFeedback', Window, 1, 10, 37463, "Circle Feedback", UpdateMarkerFeedback)
	PSTOptSlider ('FeedbackOptions', 'LocatorFeedback', Window, 2, 11, 37586, "Locator Feedback Level")
	PSTOptSlider ('FeedbackOptions', 'SelectionFeedbackLevel', Window, 20, 21, 54879, "Selection Sounds Frequency")
	PSTOptSlider ('FeedbackOptions', 'CommandFeedbackLevel', Window, 22, 23, 55012, "Command Sounds Frequency")

	PSTOptCheckbox ('FeedbackOptions', 'CharacterStates', Window, 6, 15, 37594, "")
	PSTOptCheckbox ('FeedbackOptions', 'MiscellaneousMessages', Window, 17, 19, 37596, "")
	PSTOptCheckbox ('FeedbackOptions', 'ToHitRolls', Window, 3, 12, 37588, "")
	PSTOptCheckbox ('FeedbackOptions', 'CombatInfo', Window, 4, 13, 37590, "")
	PSTOptCheckbox ('FeedbackOptions', 'SpellCasting', Window, 5, 14, 37592, "")


	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	
def UpdateMarkerFeedback ():
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

def DisplayHelpMarkerFeedback ():
	FeedbackHelpText.SetText (37411)

def DisplayHelpLocatorFeedback ():
	FeedbackHelpText.SetText (37447)

def DisplayHelpSelectionFeedbackLevel ():
	FeedbackHelpText.SetText (54878)

def DisplayHelpCommandFeedbackLevel ():
	FeedbackHelpText.SetText (54880)

def DisplayHelpCharacterStates ():
	FeedbackHelpText.SetText (37460)

def DisplayHelpMiscellaneousMessages ():
	FeedbackHelpText.SetText (37462)

def DisplayHelpToHitRolls ():
	FeedbackHelpText.SetText (37453)

def DisplayHelpCombatInfo ():
	FeedbackHelpText.SetText (37457)

def DisplayHelpSpellCasting ():
	FeedbackHelpText.SetText (37458)


###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global AutopauseOptionsWindow, AutopauseHelpText
	
	GemRB.HideGUI ()

	if AutopauseOptionsWindow:
		if AutopauseOptionsWindow:
			AutopauseOptionsWindow.Unload ()
		AutopauseOptionsWindow = None
		GemRB.SetVar ("FloatWindow", GameplayOptionsWindow.ID)
		
		GemRB.UnhideGUI ()
		GameplayOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
		return

	
	AutopauseOptionsWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("FloatWindow", AutopauseOptionsWindow.ID)


	AutopauseHelpText = GUIOPTControls.OptHelpText ('AutopauseOptions', Window, 1, 31214)

	GUIOPTControls.OptDone (OpenAutopauseOptionsWindow, Window, 16)
	GUIOPTControls.OptCancel (OpenAutopauseOptionsWindow, Window, 17)

	# Set variable for each checkbox according to a particular bit of
	#   AutoPauseState
	state = GemRB.GetVar ("Auto Pause State")
	GemRB.SetVar("AutoPauseState_Unusable", (state & 0x01) != 0 )
	GemRB.SetVar("AutoPauseState_Attacked", (state & 0x02) != 0 )
	GemRB.SetVar("AutoPauseState_Hit", (state & 0x04) != 0 )
	GemRB.SetVar("AutoPauseState_Wounded", (state & 0x08) != 0 )
	GemRB.SetVar("AutoPauseState_Dead", (state & 0x10) != 0 )
	GemRB.SetVar("AutoPauseState_NoTarget", (state & 0x20) != 0 )
	GemRB.SetVar("AutoPauseState_EndRound", (state & 0x40) != 0 )
	

	PSTOptCheckbox ('AutopauseOptions', 'CharacterHit', Window, 2, 9, 37598, "AutoPauseState_Hit", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'CharacterInjured', Window, 3, 10, 37681, "AutoPauseState_Wounded", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'CharacterDead', Window, 4, 11, 37682, "AutoPauseState_Dead", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'CharacterAttacked', Window, 5, 12, 37683, "AutoPauseState_Attacked", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'WeaponUnusable', Window, 6, 13, 37684, "AutoPauseState_Unusable", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'TargetGone', Window, 7, 14, 37685, "AutoPauseState_NoTarget", OnAutoPauseClicked)
	PSTOptCheckbox ('AutopauseOptions', 'EndOfRound', Window, 8, 15, 37686, "AutoPauseState_EndRound", OnAutoPauseClicked)

	GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)



def OnAutoPauseClicked ():
	state = (0x01 * GemRB.GetVar("AutoPauseState_Unusable") +
		 0x02 * GemRB.GetVar("AutoPauseState_Attacked") + 
		 0x04 * GemRB.GetVar("AutoPauseState_Hit") +
		 0x08 * GemRB.GetVar("AutoPauseState_Wounded") +
		 0x10 * GemRB.GetVar("AutoPauseState_Dead") +
		 0x20 * GemRB.GetVar("AutoPauseState_NoTarget") +
		 0x40 * GemRB.GetVar("AutoPauseState_EndRound"))

	GemRB.SetVar("Auto Pause State", state)
	
def DisplayHelpCharacterHit ():
	AutopauseHelpText.SetText (37688)

def DisplayHelpCharacterInjured ():
	AutopauseHelpText.SetText (37689)

def DisplayHelpCharacterDead ():
	AutopauseHelpText.SetText (37690)

def DisplayHelpCharacterAttacked ():
	AutopauseHelpText.SetText (37691)

def DisplayHelpWeaponUnusable ():
	AutopauseHelpText.SetText (37692)

def DisplayHelpTargetGone ():
	AutopauseHelpText.SetText (37693)

def DisplayHelpEndOfRound ():
	AutopauseHelpText.SetText (37694)


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
	global KeysWindow
	global last_key_action

	last_key_action = None

	GemRB.HideGUI()

	if KeysWindow:		
		if KeysWindow:
			KeysWindow.Unload ()
		KeysWindow = None
		GemRB.SetVar ("OtherWindow", OptionsWindow.ID)
		
		GemRB.LoadWindowPack ("GUIOPT")
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIKEYS")
	KeysWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", KeysWindow.ID)


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


	#KeysWindow.SetVisible (WINDOW_VISIBLE)
	GemRB.UnhideGUI ()


def keys_setup_page (pageno):
	Window = KeysWindow


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
	
	Window = KeysWindow
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
	global MoviesWindow

	GemRB.HideGUI()

	if MoviesWindow:		
		if MoviesWindow:
			MoviesWindow.Unload ()
		MoviesWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.LoadWindowPack ("GUIOPT")
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIMOVIE")
	# FIXME: clean the window to black
	MoviesWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("FloatWindow", MoviesWindow.ID)


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
	List.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	List.SetVarAssoc ('SelectedMovie', -1)
	
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMoviesWindow)


	MovieTable = GemRB.LoadTable ("MOVIDESC")

	for i in range (MovieTable.GetRowCount ()):
		#key = MovieTable.GetRowName (i)
		desc = MovieTable.GetValue (i, 0)
		List.Append (desc, i)



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
###################################################

# These functions help to setup controls found
#   in Video, Audio, Gameplay, Feedback and Autopause
#   options windows

# These controls are usually made from an active
#   control (button, slider ...) and a label

def PSTOptSlider (winname, ctlname, window, slider_id, label_id, label_strref, variable, action = None, value = 1):
	"""Standard slider for option windows"""
	slider = GUIOPTControls.OptSlider (action, window, slider_id, variable, value)
	#slider.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, eval("DisplayHelp" + ctlname))
	#slider.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))
	
	label = window.GetControl (label_id)
	label.SetText (label_strref)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)
	#label.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, eval("DisplayHelp" + ctlname))
	label.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, eval("DisplayHelp" + ctlname))
	label.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))

	return slider

def PSTOptCheckbox (winname, ctlname, window, button_id, label_id, label_strref, assoc_var = None, handler = None):
	"""Standard checkbox for option windows"""

	action = eval("DisplayHelp" + ctlname)
	button = GUIOPTControls.OptCheckbox(action, window, button_id, label_id, assoc_var, 1)
	button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))
	if assoc_var:
		if GemRB.GetVar (assoc_var):
			button.SetState (IE_GUI_BUTTON_PRESSED)
		else:
			button.SetState (IE_GUI_BUTTON_UNPRESSED)
	else: 
		button.SetState (IE_GUI_BUTTON_UNPRESSED)

	if handler:
		button.SetEvent (IE_GUI_BUTTON_ON_PRESS, handler)

	label = window.GetControl (label_id)
	label.SetText (label_strref)
	label.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, action)
	label.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))

	return button

def PSTOptButton (winname, ctlname, window, button_id, label_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = window.GetControl (button_id)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("Open%sWindow" %ctlname))
	button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, eval("DisplayHelp" + ctlname))
	button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))

	label = window.GetControl (label_id)
	label.SetText (label_strref)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)
	label.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, eval("DisplayHelp" + ctlname))
	label.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, eval("DisplayHelp" + winname))

###################################################
# End of file GUIOPT.py

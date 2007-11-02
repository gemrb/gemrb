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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# $Id$


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
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUISAVE import *
#import MessageWindow

###################################################
OptionsWindow = None
GameOptionsWindow = None
MoviesWindow = None
KeysWindow = None
HelpTextArea = None

LoadMsgWindow = None
QuitMsgWindow = None

def CloseOptionsWindow ():
	global GameOptionsWindow

	if GameOptionsWindow == None:
		return

	GemRB.UnloadWindow (GameOptionsWindow)

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVisible (0,1)
	GemRB.UnhideGUI ()
	return

###################################################
def OpenOptionsWindow ():
	"""Open main options window"""
	global OptionsWindow

	if CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow()

		#GemRB.HideGUI ()
		#GemRB.UnloadWindow (OptionsWindow)
		#OptionsWindow = None
		#GemRB.SetVar ("OtherWindow", -1)

		#GemRB.UnhideGUI ()
		return

	GemRB.HideGUI ()
	GemRB.LoadWindowPack ("GUIOPT")
	OptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", OptionsWindow)


	# Load game
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 13729)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Save Game
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 13730)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSaveMsgWindow")

	# Quit Game
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetText (Window, Button, 13731)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# Graphics
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 17162)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")

	# Audio
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 17164)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")

	# Gameplay
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 17165)
	#GemRB.SetText (Window, Button, 10308)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")

	# Return to game
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 10308)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# game version, e.g. v1.1.0000
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, GEMRB_VERSION)

	GemRB.UnhideGUI ()



###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global GameOptionsWindow, HelpTextArea

	GemRB.HideGUI ()

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return


	GameOptionsWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("FloatWindow", GameOptionsWindow)


	HelpTextArea = OptHelpText ('VideoOptions', Window, 33, 18038)

	OptDone ('VideoOptions', Window, 21)
	OptCancel ('VideoOptions', Window, 32)

	OptSlider ('Brightness', Window, 3, 'Brightness Correction', 4)
	OptSlider ('Contrast', Window, 22, 'Gamma Correction', 1)

	OptRadio ('BPP', Window, 5, 37, 'BitsPerPixel', 16)
	OptRadio ('BPP', Window, 6, 37, 'BitsPerPixel', 24)
	OptRadio ('BPP', Window, 7, 37, 'BitsPerPixel', 32)
	OptCheckbox ('FullScreen', Window, 9, 38, 'Full Screen', 1)

	OptCheckbox ('TransShadow', Window, 51, 50, 'Translucent Shadows', 1)
	OptCheckbox ('SoftMirrBlt', Window, 40, 44, 'SoftMirrorBlt' ,1)
	OptCheckbox ('SoftTransBlt', Window, 41, 46, 'SoftSrcKeyBlt' ,1)
	OptCheckbox ('SoftStandBlt', Window, 42, 48, 'SoftBltFast' ,1)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpFullScreen ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17205)

def DisplayHelpBrightness ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17203)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpContrast ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpSoftMirrBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18004)

def DisplayHelpSoftTransBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18006)

def DisplayHelpSoftStandBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18007)

def DisplayHelpTransShadow ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 20620)


###################################################

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow: OpenTalkOptionsWindow()

		GemRB.HideGUI ()
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return


	GemRB.HideGUI ()
	GameOptionsWindow = Window = GemRB.LoadWindow (7)
	GemRB.SetVar ("FloatWindow", GameOptionsWindow)


	HelpTextArea = OptHelpText ('AudioOptions', Window, 14, 18040)

	OptDone ('AudioOptions', Window, 24)
	OptCancel ('AudioOptions', Window, 25)
	OptButton ('CharacterSounds', Window, 13, 17778)

	OptSlider ('AmbientVolume', Window, 1, 'Volume Ambients', 10)
	OptSlider ('SoundFXVolume', Window, 2, 'Volume SFX', 10)
	OptSlider ('VoiceVolume', Window, 3, 'Volume Voices', 10)
	OptSlider ('MusicVolume', Window, 4, 'Volume Music', 10)
	OptSlider ('MovieVolume', Window, 22, 'Volume Movie', 10)

	OptCheckbox ('CreativeEAX', Window, 26, 28, 'Environmental Audio', 1)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpAmbientVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31227)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpSoundFXVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31228)

def DisplayHelpVoiceVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31226)

def DisplayHelpMusicVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31225)
	GemRB.UpdateMusicVolume ()

def DisplayHelpMovieVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31229)

def DisplayHelpCreativeEAX ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31224)

###################################################

def CloseCharacterSoundsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()

def OpenCharacterSoundsWindow ():
	"""Open audio chars options window"""
	global GameOptionsWindow, HelpTextArea

	GemRB.HideGUI ()

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (12)
	GemRB.SetVar ("FloatWindow", GameOptionsWindow)


	HelpTextArea = OptHelpText ('TalkOptions', Window, 16, 18041)

	OptDone ('TalkOptions', Window, 24)
	OptCancel ('TalkOptions', Window, 25)

## 	OptCheckbox ('CharacterHit', Window, 2, 9, 37598)
## 	OptCheckbox ('CharacterInjured', Window, 3, 10, 37681)
## 	OptCheckbox ('CharacterDead', Window, 4, 11, 37682)
## 	OptCheckbox ('CharacterAttacked', Window, 5, 12, 37683)
## 	OptCheckbox ('WeaponUnusable', Window, 6, 13, 37684)
## 	OptCheckbox ('TargetGone', Window, 7, 14, 37685)
## 	OptCheckbox ('EndOfRound', Window, 8, 15, 37686)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow()

		GemRB.HideGUI ()
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None
		GemRB.SetVar ("FloatWindow", -1)

		GemRB.UnhideGUI ()
		return


	GemRB.HideGUI ()
	GameOptionsWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("FloatWindow", GameOptionsWindow)


	HelpTextArea = OptHelpText ('GameplayOptions', Window, 40, 18042)

	OptDone ('GameplayOptions', Window, 7)
	OptCancel ('GameplayOptions', Window, 20)

	OptSlider ('TooltipDelay', Window, 1, 'Tooltips', 0)
	OptSlider ('MouseScrollingSpeed', Window, 2, 'Mouse Scroll Speed', 0)
	OptSlider ('KeyboardScrollingSpeed', Window, 3, 'Keyboard Scroll Speed', 0)
	OptSlider ('Difficulty', Window, 12, 'Difficulty Level', 0)

	OptCheckbox ('DitherAlways', Window, 14, 25, 'Always Dither', 1)
	OptCheckbox ('Gore', Window, 19, 27, 'Gore', 1)

	OptButton ('FeedbackOptions', Window, 5, 17163)
	OptButton ('AutopauseOptions', Window, 6, 17166)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpTooltipDelay ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31232)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpMouseScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31230)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

def DisplayHelpKeyboardScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31231)

def DisplayHelpDifficulty ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31233)

def DisplayHelpDitherAlways ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31222)

def DisplayHelpGore ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31223)

def DisplayHelpFeedbackOptions ():
	GemRB.SetText (GameplayOptionsWindow, HelpTextArea, 31213)

def DisplayHelpAutopauseOptions ():
	GemRB.SetText (GameplayOptionsWindow, HelpTextArea, 31214)


###################################################

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global FeedbackOptionsWindow, HelpTextArea

	GemRB.HideGUI ()

	if FeedbackOptionsWindow:
		GemRB.UnloadWindow (FeedbackOptionsWindow)
		FeedbackOptionsWindow = None
		GemRB.SetVar ("FloatWindow", GameplayOptionsWindow)

		GemRB.UnhideGUI ()
		GemRB.ShowModal (GameplayOptionsWindow, MODAL_SHADOW_GRAY)
		return


	FeedbackOptionsWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("FloatWindow", FeedbackOptionsWindow)


	HelpTextArea = OptHelpText ('FeedbackOptions', Window, 28, 18043)

	OptDone ('FeedbackOptions', Window, 26)
	OptCancel ('FeedbackOptions', Window, 27)

## 	OptSlider ('MarkerFeedback', Window, 1, 10, 37463)
## 	OptSlider ('LocatorFeedback', Window, 2, 11, 37586)
## 	OptSlider ('SelectionFeedbackLevel', Window, 20, 21, 54879)
## 	OptSlider ('CommandFeedbackLevel', Window, 22, 23, 55012)

## 	OptCheckbox ('CharacterStates', Window, 6, 15, 37594)
## 	OptCheckbox ('MiscellaneousMessages', Window, 17, 19, 37596)
## 	OptCheckbox ('ToHitRolls', Window, 3, 12, 37588)
## 	OptCheckbox ('CombatInfo', Window, 4, 13, 37590)
## 	OptCheckbox ('SpellCasting', Window, 5, 14, 37592)


	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpMarkerFeedback ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37411)

def DisplayHelpLocatorFeedback ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37447)

def DisplayHelpSelectionFeedbackLevel ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 54878)

def DisplayHelpCommandFeedbackLevel ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 54880)

def DisplayHelpCharacterStates ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37460)

def DisplayHelpMiscellaneousMessages ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37462)

def DisplayHelpToHitRolls ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37453)

def DisplayHelpCombatInfo ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37457)

def DisplayHelpSpellCasting ():
	GemRB.SetText (FeedbackOptionsWindow, HelpTextArea, 37458)


###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global GameOptionsWindow, HelpTextArea

	GemRB.HideGUI ()

	if AutopauseOptionsWindow:
		GemRB.UnloadWindow (AutopauseOptionsWindow)
		AutopauseOptionsWindow = None
		GemRB.SetVar ("FloatWindow", GameplayOptionsWindow)

		GemRB.UnhideGUI ()
		GemRB.ShowModal (GameplayOptionsWindow, MODAL_SHADOW_GRAY)
		return


	GameOptionsWindow = Window = GemRB.LoadWindow (10)
	GemRB.SetVar ("FloatWindow", AutopauseOptionsWindow)


	HelpTextArea = OptHelpText ('AutopauseOptions', Window, 15, 18044)

	OptDone ('AutopauseOptions', Window, 11)
	OptCancel ('AutopauseOptions', Window, 14)

	OptCheckbox ('CharacterHit', Window, 1, 17, 'Auto Pause State', 1)
	OptCheckbox ('CharacterInjured', Window, 2, 18, 'Auto Pause State', 2)
	OptCheckbox ('CharacterDead', Window, 3, 19, 'Auto Pause State', 4)
	OptCheckbox ('CharacterAttacked', Window, 4, 20, 'Auto Pause State', 8)
	OptCheckbox ('WeaponUnusable', Window, 5, 21, 'Auto Pause State', 16)
	OptCheckbox ('TargetGone', Window, 13, 22, 'Auto Pause State', 32)
	OptCheckbox ('EndOfRound', Window, 25, 24, 'Auto Pause State', 64)
	OptCheckbox ('EnemySighted', Window, 26, 27, 'Auto Pause State', 128)

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpCharacterHit ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37688)

def DisplayHelpCharacterInjured ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37689)

def DisplayHelpCharacterDead ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37690)

def DisplayHelpCharacterAttacked ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37691)

def DisplayHelpWeaponUnusable ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37692)

def DisplayHelpTargetGone ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37693)

def DisplayHelpEndOfRound ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 37694)

def DisplayHelpEnemySighted ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 23514)


###################################################

def OpenSaveMsgWindow ():
	GemRB.SetVar("QuitAfterSave",0)
	OpenSaveWindow ()
	#save the game without quitting
	return

###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		return

	LoadMsgWindow = Window = GemRB.LoadWindow (4)

	# Load
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15590)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseLoadMsgWindow")

	# Current game will be destroyed ...
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 19531)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	return

def LoadGamePress ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("GUILOAD")

def SaveGamePress():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	OpenSaveWindow ()
	return

def QuitGamePress():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("Start")
	return

###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		return

	QuitMsgWindow = Window = GemRB.LoadWindow (5)

	# Save
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 15589)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")

	# Quit Game
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 15417)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "QuitGamePress")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 13727)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseQuitMsgWindow")

	# Do you wish to save the game ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 16456) # or ??? - cannot be saved atm

	GemRB.UnhideGUI ()
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	return


###################################################

def OptSlider (name, window, slider_id, variable, value):
	"""Standard slider for option windows"""
	slider = GemRB.GetControl (window, slider_id)
	GemRB.SetVarAssoc (window, slider, variable, value)
	GemRB.SetEvent (window, slider, IE_GUI_SLIDER_ON_CHANGE, "DisplayHelp" + name)

	#label = GemRB.GetControl (window, label_id)
	#GemRB.SetText (window, label, label_strref)
	#GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	#GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)
	#GemRB.SetEvent (window, label, IE_GUI_MOUSE_ENTER_BUTTON, "DisplayHelp" + name)

	return slider

def OptRadio (name, window, button_id, label_id, variable, value):
	"""Standard radio button for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)

	return button

def OptCheckbox (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetButtonState (window, button, IE_GUI_BUTTON_SELECTED)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)
	return button

def OptButton (name, window, button_id, button_strref):
	"""Standard subwindow button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)

	GemRB.SetText (window, button, button_strref)


def OptDone (name, window, button_id):
	"""Standard `Done' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 11973) # Done
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 13727) # Cancel
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = GemRB.GetControl (window, text_id)
	GemRB.SetText (window, text, text_strref)
	return text


###################################################
# End of file GUIOPT.py

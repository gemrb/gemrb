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
import GUICommonWindows
from GUIDefines import *
from GUICommon import CloseOtherWindow
from GUISAVE import *
from GUICommonWindows import *
 
###################################################
SubOptionsWindow = None
SubSubOptionsWindow = None
GameOptionsWindow = None
OldOptionsWindow = None
MoviesWindow = None
KeysWindow = None
HelpTextArea = None
HelpTextArea2 = None

LoadMsgWindow = None
QuitMsgWindow = None
hideflag = None


def CloseOptionsWindow ():
	global GameOptionsWindow, OptionsWindow
	global OldOptionsWindow

	if GameOptionsWindow == None:
		return

	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)

	GameOptionsWindow = None
	SetSelectionChangeHandler (None)
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVisible (0,1)
	OptionsWindow = OldOptionsWindow
	OldOptionsWindow = None
	return

def DummyWindow ():
	return

###################################################
def OpenOptionsWindow ():
	"""Open main options window"""
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldOptionsWindow

	if CloseOtherWindow(OpenOptionsWindow):
		CloseOptionsWindow()
		return

	#hideflag = GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	SetSelectionChangeHandler (DummyWindow)

	GemRB.LoadWindowPack ("GUIOPT", 640, 480)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow)
	if OldOptionsWindow == None:
		OldOptionsWindow = OptionsWindow
		OptionsWindow = GemRB.LoadWindow (0)
		SetupMenuWindowControls (OptionsWindow, 0, "OpenOptionsWindow")
		GemRB.SetWindowFrame (OptionsWindow)

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

	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return


###################################################
def CloseVideoOptionsWindow ():
	global SubOptionsWindow

	GemRB.UnloadWindow(SubOptionsWindow)
	SubOptionsWindow = None
	return

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		GemRB.UnloadWindow(SubOptionsWindow)
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindow (6)

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

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpFullScreen ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 17205)

def DisplayHelpBrightness ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 17203)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpContrast ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpSoftMirrBlt ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18004)

def DisplayHelpSoftTransBlt ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18006)

def DisplayHelpSoftStandBlt ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18007)

def DisplayHelpTransShadow ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 20620)


###################################################

def CloseAudioOptionsWindow ():
	global SubOptionsWindow

	GemRB.UnloadWindow(SubOptionsWindow)
	SubOptionsWindow = None
	return

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		GemRB.UnloadWindow (SubOptionsWindow)
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindow (7)

	HelpTextArea = OptHelpText ('AudioOptions', Window, 14, 18040)

	GemRB.DeleteControl(Window, 16)

	OptDone ('AudioOptions', Window, 24)
	OptCancel ('AudioOptions', Window, 25)
	OptButton ('CharacterSounds', Window, 13, 17778)

	OptSlider ('AmbientVolume', Window, 1, 'Volume Ambients', 10)
	OptSlider ('SoundFXVolume', Window, 2, 'Volume SFX', 10)
	OptSlider ('VoiceVolume', Window, 3, 'Volume Voices', 10)
	OptSlider ('MusicVolume', Window, 4, 'Volume Music', 10)
	OptSlider ('MovieVolume', Window, 22, 'Volume Movie', 10)

	OptCheckbox ('CreativeEAX', Window, 26, 28, 'Environmental Audio', 1)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def DisplayHelpAmbientVolume ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18008)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpSoundFXVolume ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18009)

def DisplayHelpVoiceVolume ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18010)

def DisplayHelpMusicVolume ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18011)
	GemRB.UpdateMusicVolume ()

def DisplayHelpMovieVolume ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18012)

def DisplayHelpCreativeEAX ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18022)

###################################################

def CloseCharacterSoundsWindow ():
	global SubSubOptionsWindow

	GemRB.UnloadWindow (SubSubOptionsWindow)
	SubSubOptionsWindow = None
	return

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		GemRB.UnloadWindow (SubSubOptionsWindow)
		SubSubOptionsWindow = None

	SubSubOptionsWindow = Window = GemRB.LoadWindow (12)

	HelpTextArea2 = OptHelpText ('CharacterSounds', Window, 16, 18041)

	OptDone ('CharacterSounds', Window, 24)
	OptCancel ('CharacterSounds', Window, 25)

	OptCheckbox ('Subtitles', Window, 5, 20, 'Subtitles', 1)
	OptCheckbox ('AttackSounds', Window, 6, 18, 'Attack Sounds', 1)
	OptCheckbox ('Footsteps', Window, 7, 19, 'Footsteps', 1)
	OptRadio ('CommandSounds', Window, 8, 21, 'Command Sounds Frequency', 1)
	OptRadio ('CommandSounds', Window, 9, 21, 'Command Sounds Frequency', 2)
	OptRadio ('CommandSounds', Window, 10, 21, 'Command Sounds Frequency', 3)
	OptRadio ('SelectionSounds', Window, 58, 57, 'Selection Sounds Frequency', 1)
	OptRadio ('SelectionSounds', Window, 59, 57, 'Selection Sounds Frequency', 2)
	OptRadio ('SelectionSounds', Window, 60, 57, 'Selection Sounds Frequency', 3)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY) 
###################################################

def CloseGameplayOptionsWindow ():
	global SubOptionsWindow
 
	GemRB.UnloadWindow(SubOptionsWindow)
	SubOptionsWindow = None
	return
  
def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		GemRB.UnloadWindow (SubOptionsWindow)
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindow (8)

	HelpTextArea = OptHelpText ('GameplayOptions', Window, 40, 18042)

	OptDone ('GameplayOptions', Window, 7)
	OptCancel ('GameplayOptions', Window, 20)

	OptSlider ('TooltipDelay', Window, 1, 'Tooltips', TOOLTIP_DELAY_FACTOR)
	OptSlider ('MouseScrollingSpeed', Window, 2, 'Mouse Scroll Speed', 5)
	OptSlider ('KeyboardScrollingSpeed', Window, 3, 'Keyboard Scroll Speed', 5)
	OptSlider ('Difficulty', Window, 12, 'Difficulty Level', 0)

	OptCheckbox ('DitherAlways', Window, 14, 25, 'Always Dither', 1)
	OptCheckbox ('Gore', Window, 19, 27, 'Gore', 1)
	OptCheckbox ('Infravision', Window, 42, 44, 'Infravision', 1)
	OptCheckbox ('Weather', Window, 47, 46, 'Weather', 1) 

	OptButton ('FeedbackOptions', Window, 5, 17163)
	OptButton ('AutopauseOptions', Window, 6, 17166)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpTooltipDelay ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpMouseScrollingSpeed ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

def DisplayHelpKeyboardScrollingSpeed ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18019)

def DisplayHelpDifficulty ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18020)

def DisplayHelpDitherAlways ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18021)

def DisplayHelpGore ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 18023)

def DisplayHelpInfravision ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 11797) 

def DisplayHelpWeather ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 20619)
 
def DisplayHelpRestUntilHealed ():
	GemRB.SetText (SubOptionsWindow, HelpTextArea, 2242) 


###################################################
def CloseFeedbackOptionsWindow ():
	global SubSubOptionsWindow

	GemRB.UnloadWindow (SubSubOptionsWindow)
	SubSubOptionsWindow = None
	return

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		GemRB.UnloadWindow (SubSubOptionsWindow)
		SubSubOptionsWindow = None 

	SubSubOptionsWindow = Window = GemRB.LoadWindow (9)

	HelpTextArea2 = OptHelpText ('FeedbackOptions', Window, 28, 18043)

	OptDone ('FeedbackOptions', Window, 26)
	OptCancel ('FeedbackOptions', Window, 27)

	OptSlider ('MarkerFeedback', Window, 8, 'GUI Feedback Level', 1)
	OptSlider ('LocatorFeedback', Window, 9, 'Locator Feedback Level', 1) 
	OptCheckbox ('ToHitRolls', Window, 10, 32, 'Rolls', 1)
	OptCheckbox ('CombatInfo', Window, 11, 33, 'Combat Info', 1)
	OptCheckbox ('Actions', Window, 12, 34, 'Actions', 1)
	OptCheckbox ('States', Window, 13, 35, 'State Changes', 1)
	OptCheckbox ('Selection', Window, 14, 36, 'Selection Text', 1)
	OptCheckbox ('Miscellaneous', Window, 15, 37, 'Miscellaneous Text', 1) 

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpMarkerFeedback ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18024)

def DisplayHelpLocatorFeedback ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18025)

def DisplayHelpToHitRolls ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18026)

def DisplayHelpCombatInfo ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18027)

def DisplayHelpActions ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18028)

def DisplayHelpStates ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18029)

def DisplayHelpSelection ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18030)

def DisplayHelpMiscellaneous ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea2, 18031)

###################################################

def CloseAutopauseOptionsWindow ():
	global SubSubOptionsWindow

	GemRB.UnloadWindow (SubSubOptionsWindow)
	SubSubOptionsWindow = None
	return

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		GemRB.UnloadWindow (SubSubOptionsWindow)
		SubSubOptionsWindow = None 

	SubSubOptionsWindow = Window = GemRB.LoadWindow (10)

	HelpTextArea2 = OptHelpText ('AutopauseOptions', Window, 15, 18044)

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

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)


def DisplayHelpCharacterHit ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18032)

def DisplayHelpCharacterInjured ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18033)

def DisplayHelpCharacterDead ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18034)

def DisplayHelpCharacterAttacked ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18035)

def DisplayHelpWeaponUnusable ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18036)

def DisplayHelpTargetGone ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 18037)

def DisplayHelpEndOfRound ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 10640)

def DisplayHelpEnemySighted ():
	GemRB.SetText (SubSubOptionsWindow, HelpTextArea, 23514)


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

	# Loading a game will destroy ...
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
	return

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

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	return


###################################################
# These functions help to setup controls found
# in Video, Audio, Gameplay, Feedback and Autopause
# options windows

# These controls are usually made from an active
# control (button, slider ...) and a label

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
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 13727) # Cancel
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = GemRB.GetControl (window, text_id)
	GemRB.SetText (window, text, text_strref)
	return text


###################################################
# End of file GUIOPT.py

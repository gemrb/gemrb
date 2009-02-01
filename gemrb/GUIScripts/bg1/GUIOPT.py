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

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	if OptionsWindow:
		OptionsWindow.Unload ()

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
	global GameOptionsWindow, OptionsWindow
	global OldOptionsWindow

	if CloseOtherWindow(OpenOptionsWindow):
		CloseOptionsWindow()
		return

	#hideflag = GemRB.HideGUI ()
	GemRB.SetVisible (0,0)
	SetSelectionChangeHandler (DummyWindow)

	GemRB.LoadWindowPack ("GUIOPT", 640, 480)
	GameOptionsWindow = Window = GemRB.LoadWindowObject (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow.ID)
	if OldOptionsWindow == None:
		OldOptionsWindow = OptionsWindow
		OptionsWindow = GemRB.LoadWindowObject (0)
		SetupMenuWindowControls (OptionsWindow, 0, "OpenOptionsWindow")
		OptionsWindow.SetFrame ()

	# Load game
	Button = Window.GetControl (5)
	Button.SetText (13729)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Save Game
	Button = Window.GetControl (6)
	Button.SetText (13730)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenSaveMsgWindow")

	# Quit Game
	Button = Window.GetControl (10)
	Button.SetText (13731)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# Graphics
	Button = Window.GetControl (7)
	Button.SetText (17162)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")

	# Audio
	Button = Window.GetControl (8)
	Button.SetText (17164)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")

	# Gameplay
	Button = Window.GetControl (9)
	Button.SetText (17165)
	#Button.SetText (10308)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")

	# Return to game
	Button = Window.GetControl (11)
	Button.SetText (10308)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	# game version, e.g. v1.1.0000
	Label = Window.GetControl (0x1000000b)
	Label.SetText (GEMRB_VERSION)

	OptionsWindow.SetVisible (1)
	Window.SetVisible (1)
	GUICommonWindows.PortraitWindow.SetVisible (1)
	return


###################################################
def CloseVideoOptionsWindow ():
	global SubOptionsWindow

	if SubOptionsWindow:
		SubOptionsWindow.Unload()
	SubOptionsWindow = None
	return

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		if SubOptionsWindow:
			SubOptionsWindow.Unload()
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindowObject (6)

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

	Window.ShowModal (MODAL_SHADOW_GRAY)


def DisplayHelpFullScreen ():
	HelpTextArea.SetText (18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	HelpTextArea.SetText (17205)

def DisplayHelpBrightness ():
	HelpTextArea.SetText (17203)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpContrast ():
	HelpTextArea.SetText (17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)

def DisplayHelpSoftMirrBlt ():
	HelpTextArea.SetText (18004)

def DisplayHelpSoftTransBlt ():
	HelpTextArea.SetText (18006)

def DisplayHelpSoftStandBlt ():
	HelpTextArea.SetText (18007)

def DisplayHelpTransShadow ():
	HelpTextArea.SetText (20620)


###################################################

def CloseAudioOptionsWindow ():
	global SubOptionsWindow

	if SubOptionsWindow:
		SubOptionsWindow.Unload()
	SubOptionsWindow = None
	return

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		if SubOptionsWindow:
			SubOptionsWindow.Unload ()
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindowObject (7)

	HelpTextArea = OptHelpText ('AudioOptions', Window, 14, 18040)

	Window.DeleteControl(16)

	OptDone ('AudioOptions', Window, 24)
	OptCancel ('AudioOptions', Window, 25)
	OptButton ('CharacterSounds', Window, 13, 17778)

	OptSlider ('AmbientVolume', Window, 1, 'Volume Ambients', 10)
	OptSlider ('SoundFXVolume', Window, 2, 'Volume SFX', 10)
	OptSlider ('VoiceVolume', Window, 3, 'Volume Voices', 10)
	OptSlider ('MusicVolume', Window, 4, 'Volume Music', 10)
	OptSlider ('MovieVolume', Window, 22, 'Volume Movie', 10)

	OptCheckbox ('CreativeEAX', Window, 26, 28, 'Environmental Audio', 1)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpAmbientVolume ():
	HelpTextArea.SetText (18008)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpSoundFXVolume ():
	HelpTextArea.SetText (18009)

def DisplayHelpVoiceVolume ():
	HelpTextArea.SetText (18010)

def DisplayHelpMusicVolume ():
	HelpTextArea.SetText (18011)
	GemRB.UpdateMusicVolume ()

def DisplayHelpMovieVolume ():
	HelpTextArea.SetText (18012)

def DisplayHelpCreativeEAX ():
	HelpTextArea.SetText (18022)

###################################################

def CloseCharacterSoundsWindow ():
	global SubSubOptionsWindow

	if SubSubOptionsWindow:
		SubSubOptionsWindow.Unload ()
	SubSubOptionsWindow = None
	return

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None

	SubSubOptionsWindow = Window = GemRB.LoadWindowObject (12)

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

	Window.ShowModal (MODAL_SHADOW_GRAY) 
###################################################

def CloseGameplayOptionsWindow ():
	global SubOptionsWindow
 
	if SubOptionsWindow:
		SubOptionsWindow.Unload()
	SubOptionsWindow = None
	return
  
def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global SubOptionsWindow, HelpTextArea

	if SubOptionsWindow:
		if SubOptionsWindow:
			SubOptionsWindow.Unload ()
		SubOptionsWindow = None 

	SubOptionsWindow = Window = GemRB.LoadWindowObject (8)

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

	Window.ShowModal (MODAL_SHADOW_GRAY)


def DisplayHelpTooltipDelay ():
	HelpTextArea.SetText (18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpMouseScrollingSpeed ():
	HelpTextArea.SetText (18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

def DisplayHelpKeyboardScrollingSpeed ():
	HelpTextArea.SetText (18019)

def DisplayHelpDifficulty ():
	HelpTextArea.SetText (18020)

def DisplayHelpDitherAlways ():
	HelpTextArea.SetText (18021)

def DisplayHelpGore ():
	HelpTextArea.SetText (18023)

def DisplayHelpInfravision ():
	HelpTextArea.SetText (11797) 

def DisplayHelpWeather ():
	HelpTextArea.SetText (20619)
 
def DisplayHelpRestUntilHealed ():
	HelpTextArea.SetText (2242) 


###################################################
def CloseFeedbackOptionsWindow ():
	global SubSubOptionsWindow

	if SubSubOptionsWindow:
		SubSubOptionsWindow.Unload ()
	SubSubOptionsWindow = None
	return

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None 

	SubSubOptionsWindow = Window = GemRB.LoadWindowObject (9)

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

	Window.ShowModal (MODAL_SHADOW_GRAY)


def DisplayHelpMarkerFeedback ():
	HelpTextArea.SetText (18024)

def DisplayHelpLocatorFeedback ():
	HelpTextArea.SetText (18025)

def DisplayHelpToHitRolls ():
	HelpTextArea2.SetText (18026)

def DisplayHelpCombatInfo ():
	HelpTextArea2.SetText (18027)

def DisplayHelpActions ():
	HelpTextArea2.SetText (18028)

def DisplayHelpStates ():
	HelpTextArea2.SetText (18029)

def DisplayHelpSelection ():
	HelpTextArea2.SetText (18030)

def DisplayHelpMiscellaneous ():
	HelpTextArea2.SetText (18031)

###################################################

def CloseAutopauseOptionsWindow ():
	global SubSubOptionsWindow

	if SubSubOptionsWindow:
		SubSubOptionsWindow.Unload ()
	SubSubOptionsWindow = None
	return

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global SubSubOptionsWindow, HelpTextArea2

	if SubSubOptionsWindow:
		if SubSubOptionsWindow:
			SubSubOptionsWindow.Unload ()
		SubSubOptionsWindow = None 

	SubSubOptionsWindow = Window = GemRB.LoadWindowObject (10)

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

	Window.ShowModal (MODAL_SHADOW_GRAY)


def DisplayHelpCharacterHit ():
	HelpTextArea.SetText (18032)

def DisplayHelpCharacterInjured ():
	HelpTextArea.SetText (18033)

def DisplayHelpCharacterDead ():
	HelpTextArea.SetText (18034)

def DisplayHelpCharacterAttacked ():
	HelpTextArea.SetText (18035)

def DisplayHelpWeaponUnusable ():
	HelpTextArea.SetText (18036)

def DisplayHelpTargetGone ():
	HelpTextArea.SetText (18037)

def DisplayHelpEndOfRound ():
	HelpTextArea.SetText (10640)

def DisplayHelpEnemySighted ():
	HelpTextArea.SetText (23514)


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

	LoadMsgWindow = Window = GemRB.LoadWindowObject (4)

	# Load
	Button = Window.GetControl (0)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "LoadGamePress")

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseLoadMsgWindow")

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (19531)

	Window.ShowModal (MODAL_SHADOW_GRAY)

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		LoadMsgWindow.Unload ()
	LoadMsgWindow = None
	#OptionsWindow.SetVisible (1)
	return

def LoadGamePress ():
	global LoadMsgWindow

	if LoadMsgWindow:
		LoadMsgWindow.Unload ()
	LoadMsgWindow = None
	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("GUILOAD")
	return

def SaveGamePress():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	OpenSaveWindow ()
	return

def QuitGamePress():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
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

	QuitMsgWindow = Window = GemRB.LoadWindowObject (5)

	# Save
	Button = Window.GetControl (0)
	Button.SetText (15589)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "SaveGamePress")

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "QuitGamePress")

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "CloseQuitMsgWindow")

	# Do you wish to save the game ....
	Text = Window.GetControl (3)
	Text.SetText (16456) # or ??? - cannot be saved atm

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	OptionsWindow.SetVisible (1)
	return


###################################################
# These functions help to setup controls found
# in Video, Audio, Gameplay, Feedback and Autopause
# options windows

# These controls are usually made from an active
# control (button, slider ...) and a label

def OptSlider (name, window, slider_id, variable, value):
	"""Standard slider for option windows"""
	slider = window.GetControl (slider_id)
	slider.SetVarAssoc (variable, value)
	slider.SetEvent (IE_GUI_SLIDER_ON_CHANGE, "DisplayHelp" + name)

	#label = window.GetControl (label_id)
	#label.SetText (label_strref)
	#label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	#label.SetState (IE_GUI_BUTTON_LOCKED)
	#label.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, "DisplayHelp" + name)

	return slider

def OptRadio (name, window, button_id, label_id, variable, value):
	"""Standard radio button for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	button.SetVarAssoc (variable, value)

	label = window.GetControl (label_id)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)

	return button

def OptCheckbox (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	button.SetState (IE_GUI_BUTTON_SELECTED)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	button.SetVarAssoc (variable, value)
	return button

def OptButton (name, window, button_id, button_strref):
	"""Standard subwindow button for option windows"""
	button = window.GetControl (button_id)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)

	button.SetText (button_strref)


def OptDone (name, window, button_id):
	"""Standard `Done' button for option windows"""
	button = window.GetControl (button_id)
	button.SetText (11973) # Done
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = window.GetControl (button_id)
	button.SetText (13727) # Cancel
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = window.GetControl (text_id)
	text.SetText (text_strref)
	return text


###################################################
# End of file GUIOPT.py

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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIOPT.py,v 1.2 2004/01/18 18:12:40 edheldil Exp $


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

from GUICommonWindows import CloseCommonWindows
import MessageWindow

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

	GemRB.HideGUI()

	if OptionsWindow:
		if VideoOptionsWindow: OpenVideoOptionsWindow ()
		if AudioOptionsWindow: OpenAudioOptionsWindow ()
		if GameplayOptionsWindow: OpenGameplayOptionsWindow ()
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow ()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow ()
		
		GemRB.UnloadWindow (OptionsWindow)
		OptionsWindow = None
		GemRB.SetVar ("OtherWindow", -1)
		
		GemRB.UnhideGUI ()
		return
		
	GemRB.LoadWindowPack ("GUIOPT")
	OptionsWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", OptionsWindow)
	
	# Return to Game
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 28638)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")	

	# Quit Game
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 2595)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# Load Game
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 2592)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Save Game
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 20639)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSaveWindow")

	# Video Options
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 28781)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")

	# Audio Options
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 29720)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")

	# Gameplay Options
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 29722)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")

	# Keyboard Mappings
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 29723)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenKeyboardMappingsWindow")

	# Movies
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 38156)   # or  2594
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenMoviesWindow")

	# game version, e.g. v1.1.0000
	Label = GemRB.GetControl (Window, 0x10000007)
	GemRB.SetText (Window, Label, GEMRB_VERSION)
	
	#GemRB.SetVisible (Window, 1)
	GemRB.UnhideGUI ()


	
###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global VideoOptionsWindow, VideoHelpText

	GemRB.HideGUI ()

	if VideoOptionsWindow:
		GemRB.UnloadWindow (VideoOptionsWindow)
		VideoOptionsWindow = None
		GemRB.SetVar ("OtherWindow", OptionsWindow)
		
		GemRB.UnhideGUI ()
		return

	
	VideoOptionsWindow = Window = GemRB.LoadWindow (1)
	GemRB.SetVar ("OtherWindow", VideoOptionsWindow)


	VideoHelpText = OptHelpText ('VideoOptions', Window, 9, 31052)

	OptDone ('VideoOptions', Window, 7)
	OptCancel ('VideoOptions', Window, 8)

	OptSlider ('Brightness', Window, 1, 10, 31234)
	OptSlider ('Contrast', Window, 2, 11, 31429)

	OptCheckbox ('SoftwareBlitting', Window, 6, 15, 30898)
	OptCheckbox ('SoftwareMirroring', Window, 4, 13, 30896)
	OptCheckbox ('SoftwareTransparency', Window, 5, 14, 30897)

	#GemRB.SetVisible (Window, 1)
	GemRb.UnhideGUI ()
	
	

def DisplayHelpBrightness ():
	GemRB.SetText (VideoOptionsWindow, VideoHelpText, 31431)

def DisplayHelpContrast ():
	GemRB.SetText (VideoOptionsWindow, VideoHelpText, 31459)

def DisplayHelpSoftwareBlitting ():
	GemRB.SetText (VideoOptionsWindow, VideoHelpText, 31221)

def DisplayHelpSoftwareMirroring ():
	GemRB.SetText (VideoOptionsWindow, VideoHelpText, 31216)

def DisplayHelpSoftwareTransparency ():
	GemRB.SetText (VideoOptionsWindow, VideoHelpText, 31220)



###################################################

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global AudioOptionsWindow, AudioHelpText

	GemRB.HideGUI ()

	if AudioOptionsWindow:
		GemRB.UnloadWindow (AudioOptionsWindow)
		AudioOptionsWindow = None
		GemRB.SetVar ("OtherWindow", OptionsWindow)
		
		GemRB.UnhideGUI ()
		return

	
	AudioOptionsWindow = Window = GemRB.LoadWindow (5)
	GemRB.SetVar ("OtherWindow", AudioOptionsWindow)
	

	AudioHelpText = OptHelpText ('AudioOptions', Window, 9, 31210)

	OptDone ('AudioOptions', Window, 7)
	OptCancel ('AudioOptions', Window, 8)

	OptSlider ('AmbientVolume', Window, 1, 10, 31460)
	OptSlider ('SoundFXVolume', Window, 2, 11, 31466)
	OptSlider ('VoiceVolume', Window, 3, 12, 31467)
	OptSlider ('MusicVolume', Window, 4, 13, 31468)
	OptSlider ('MovieVolume', Window, 5, 14, 31469)
	
	OptCheckbox ('CreativeEAX', Window, 6, 15, 30900)
	OptCheckbox ('SoundProcessing', Window, 16, 17, 63242)
	OptCheckbox ('MusicProcessing', Window, 18, 19, 63243)


	#GemRB.SetVisible (AudioOptionsWindow, 1)
	GemRB.UnhideGUI ()
	

def DisplayHelpAmbientVolume ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31227)
	
def DisplayHelpSoundFXVolume ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31228)

def DisplayHelpVoiceVolume ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31226)

def DisplayHelpMusicVolume ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31225)

def DisplayHelpMovieVolume ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31229)

def DisplayHelpCreativeEAX ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 31224)

def DisplayHelpSoundProcessing ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 63244)
	
def DisplayHelpMusicProcessing ():
	GemRB.SetText (AudioOptionsWindow, AudioHelpText, 63247)



###################################################

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameplayOptionsWindow, GameplayHelpText

	GemRB.HideGUI ()

	if GameplayOptionsWindow:
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow()

		GemRB.UnloadWindow (GameplayOptionsWindow)
		GameplayOptionsWindow = None
		GemRB.SetVar ("OtherWindow", OptionsWindow)
		
		GemRB.UnhideGUI ()
		return

	
	GameplayOptionsWindow = Window = GemRB.LoadWindow (6)
	GemRB.SetVar ("OtherWindow", GameplayOptionsWindow)
	

	GameplayHelpText = OptHelpText ('GameplayOptions', Window, 12, 31212)

	OptDone ('GameplayOptions', Window, 10)
	OptCancel ('GameplayOptions', Window, 11)

	OptSlider ('TooltipDelay', Window, 1, 13, 31481)
	OptSlider ('MouseScrollingSpeed', Window, 2, 14, 31482)
	OptSlider ('KeyboardScrollingSpeed', Window, 3, 15, 31480)
	OptSlider ('Difficulty', Window, 4, 16, 31479)

	OptCheckbox ('DitherAlways', Window, 5, 17, 31217)
	OptCheckbox ('Gore', Window, 6, 18, 31218)
	OptCheckbox ('AlwaysRun', Window, 22, 23, 62418)

	OptButton ('FeedbackOptions', Window, 8, 20, 31478)
	OptButton ('AutopauseOptions', Window, 9, 21, 31470)

	#GemRB.SetVisible (Window, 1)
	GemRB.UnhideGUI ()


def DisplayHelpTooltipDelay ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31232)

def DisplayHelpMouseScrollingSpeed ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31230)

def DisplayHelpKeyboardScrollingSpeed ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31231)

def DisplayHelpDifficulty ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31233)


def DisplayHelpDitherAlways ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31222)

def DisplayHelpGore ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31223)

def DisplayHelpAlwaysRun ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 62419)


def DisplayHelpFeedbackOptions ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31213)

def DisplayHelpAutopauseOptions ():
	GemRB.SetText (GameplayOptionsWindow, GameplayHelpText, 31214)



###################################################
	
def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global FeedbackOptionsWindow, FeedbackHelpText
	
	GemRB.HideGUI ()

	if FeedbackOptionsWindow:
		GemRB.UnloadWindow (FeedbackOptionsWindow)
		FeedbackOptionsWindow = None
		GemRB.SetVar ("OtherWindow", GameplayOptionsWindow)
		
		GemRB.UnhideGUI ()
		return

	
	FeedbackOptionsWindow = Window = GemRB.LoadWindow (8)
	GemRB.SetVar ("OtherWindow", FeedbackOptionsWindow)


	FeedbackHelpText = OptHelpText ('FeedbackOptions', Window, 9, 37410)

	OptDone ('FeedbackOptions', Window, 7)
	OptCancel ('FeedbackOptions', Window, 8)

	OptSlider ('MarkerFeedback', Window, 1, 10, 37463)
	OptSlider ('LocatorFeedback', Window, 2, 11, 37586)
	OptSlider ('SelectionFeedbackLevel', Window, 20, 21, 54879)
	OptSlider ('CommandFeedbackLevel', Window, 22, 23, 55012)

	OptCheckbox ('CharacterStates', Window, 6, 15, 37594)
	OptCheckbox ('MiscellaneousMessages', Window, 17, 19, 37596)
	OptCheckbox ('ToHitRolls', Window, 3, 12, 37588)
	OptCheckbox ('CombatInfo', Window, 4, 13, 37590)
	OptCheckbox ('SpellCasting', Window, 5, 14, 37592)


	#GemRB.SetVisible (FeedbackOptionsWindow, 1)
	GemRB.UnhideGUI ()
	

def DisplayHelpMarkerFeedback ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37411)

def DisplayHelpLocatorFeedback ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37447)

def DisplayHelpSelectionFeedbackLevel ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 54878)

def DisplayHelpCommandFeedbackLevel ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 54880)

def DisplayHelpCharacterStates ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37460)

def DisplayHelpMiscellaneousMessages ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37462)

def DisplayHelpToHitRolls ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37453)

def DisplayHelpCombatInfo ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37457)

def DisplayHelpSpellCasting ():
	GemRB.SetText (FeedbackOptionsWindow, FeedbackHelpText, 37458)


###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global AutopauseOptionsWindow, AutopauseHelpText
	
	GemRB.HideGUI ()

	if AutopauseOptionsWindow:
		GemRB.UnloadWindow (AutopauseOptionsWindow)
		AutopauseOptionsWindow = None
		GemRB.SetVar ("OtherWindow", GameplayOptionsWindow)
		
		GemRB.UnhideGUI ()
		return

	
	AutopauseOptionsWindow = Window = GemRB.LoadWindow (9)
	GemRB.SetVar ("OtherWindow", AutopauseOptionsWindow)


	AutopauseHelpText = OptHelpText ('AutopauseOptions', Window, 1, 31214)

	OptDone ('AutopauseOptions', Window, 16)
	OptCancel ('AutopauseOptions', Window, 17)

	OptCheckbox ('CharacterHit', Window, 2, 9, 37598)
	OptCheckbox ('CharacterInjured', Window, 3, 10, 37681)
	OptCheckbox ('CharacterDead', Window, 4, 11, 37682)
	OptCheckbox ('CharacterAttacked', Window, 5, 12, 37683)
	OptCheckbox ('WeaponUnusable', Window, 6, 13, 37684)
	OptCheckbox ('TargetGone', Window, 7, 14, 37685)
	OptCheckbox ('EndOfRound', Window, 8, 15, 37686)

	#GemRB.SetVisible (AutopauseOptionsWindow, 1)
	GemRB.UnhideGUI ()
	

def DisplayHelpCharacterHit ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37688)

def DisplayHelpCharacterInjured ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37689)

def DisplayHelpCharacterDead ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37690)

def DisplayHelpCharacterAttacked ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37691)

def DisplayHelpWeaponUnusable ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37692)

def DisplayHelpTargetGone ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37693)

def DisplayHelpEndOfRound ():
	GemRB.SetText (AutopauseOptionsWindow, AutopauseHelpText, 37694)


###################################################
###################################################

def OpenSaveWindow ():
	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)
	GemRB.SetNextScript ('GUISAVE')
	


###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow
	
	LoadMsgWindow = Window = GemRB.LoadWindow (3)

	
	# Load
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 28648)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "LoadGame")

	# Cancel
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseLoadMsgWindow")

	# Loading a game will destroy ...
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 39432)

	GemRB.SetVisible (Window, 1)


def CloseLoadMsgWindow ():
	global LoadMsgWindow
	
	GemRB.SetVisible (LoadMsgWindow, 0)
	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = 0
	GemRB.SetVisible (MainWindow, 1)

def LoadGame ():
	CloseLoadMsgWindow ()
	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)
	GemRB.SetNextScript ('GUILOAD')



###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow
	
	QuitMsgWindow = Window = GemRB.LoadWindow (4)

	
	# Save
	Button = GemRB.GetControl (Window, 0)
	GemRB.SetText (Window, Button, 28645)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "SaveGame")

	# Quit Game
	Button = GemRB.GetControl (Window, 1)
	GemRB.SetText (Window, Button, 2595)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "QuitGame")

	# Cancel
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseQuitMsgWindow")

	# The game has not been saved ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 39430)  # or 39431 - cannot be saved atm

	GemRB.SetVisible (Window, 1)


def CloseQuitMsgWindow ():
	global QuitMsgWindow
	
	GemRB.SetVisible (QuitMsgWindow, 0)
	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = 0
	GemRB.SetVisible (MainWindow, 1)

def QuitGame ():
	CloseQuitMsgWindow ()
	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)
	GemRB.SetNextScript ('Start')

def SaveGame ():
	CloseQuitMsgWindow ()
	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)
	GemRB.SetNextScript ('GUISAVE')


###################################################

def OpenKeyboardMappingsWindow ():
	global KeysWindow

	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)

	GemRB.LoadWindowPack ("GUIKEYS")
	KeysWindow = Window = GemRB.LoadWindow (0)

	# Page n of n
	Label = GemRB.GetControl (Window, 0x10000001)
	GemRB.SetText (Window, Label, 49053)

	# Default
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 49051)
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")	

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseKeyboardMappingsWindow")	

	# Cancel
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 4196)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseKeyboardMappingsWindow")	

	for i in range (60):
		Label = GemRB.GetControl (Window, 0x10000005 + i)
		GemRB.SetText (Window, Label, 'A')
		Label = GemRB.GetControl (Window, 0x10000041 + i)
		GemRB.SetText (Window, Label, 'Some action')
		


	GemRB.SetVisible (KeysWindow, 1)


def CloseKeyboardMappingsWindow ():
	GemRB.SetVisible (KeysWindow, 0)
	GemRB.UnloadWindow (KeysWindow)

	OpenOptionsWindow ()


###################################################

def OpenMoviesWindow ():
	global MoviesWindow

	GemRB.SetVisible (MainWindow, 0)
	GemRB.UnloadWindow (MainWindow)
	# FIXME: clean the window to black

	GemRB.LoadWindowPack ("GUIMOVIE")
	MoviesWindow = Window = GemRB.LoadWindow (0)

	# Play Movie
	Button = GemRB.GetControl (Window, 2)
	GemRB.SetText (Window, Button, 33034)
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")	

	# Credits
	Button = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Button, 33078)
	#GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "")	

	# Done
	Button = GemRB.GetControl (Window, 4)
	GemRB.SetText (Window, Button, 1403)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "CloseMoviesWindow")	


	GemRB.SetVisible (MoviesWindow, 1)

def CloseMoviesWindow ():
	GemRB.SetVisible (MoviesWindow, 0)
	GemRB.UnloadWindow (MoviesWindow)

	OpenOptionsWindow ()


###################################################
###################################################

# These functions help to setup controls found
#   in Video, Audio, Gameplay, Feedback and Autopause
#   options windows

# These controls are usually made from an active
#   control (button, slider ...) and a label


def OptSlider (name, window, slider_id, label_id, label_strref):
	"""Standard slider for option windows"""
	slider = GemRB.GetControl (window, slider_id)
	#GemRB.SetEvent (window, slider, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)
	
	label = GemRB.GetControl (window, label_id)
	GemRB.SetText (window, label, label_strref)
	#GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	return slider


def OptCheckbox (name, window, button_id, label_id, label_strref):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_SET)
	GemRB.SetButtonState (window, button, IE_GUI_BUTTON_PRESSED)
	GemRB.SetEvent (window, button, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetText (window, label, label_strref)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	return button

def OptButton (name, window, button_id, label_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)	
	GemRB.SetEvent (window, button, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetText (window, label, label_strref)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

def OptDone (name, window, button_id):
	"""Standard `Done' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 1403) # Done
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)	

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetText (window, button, 4196) # Cancel
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Close%sWindow" %name)	

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = GemRB.GetControl (window, text_id)
	GemRB.SetText (window, text, text_strref)
	return text


###################################################
# End of file GUIOPT.py

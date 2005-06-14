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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/tob/GUIOPT.py,v 1.7 2005/06/14 17:53:00 avenger_teambg Exp $

# GUIOPT.py - scripts to control options windows mostly from GUIOPT winpack
# Ingame options

###################################################
import GemRB
from GUIDefines import *
from GUICommon import CloseOtherWindow
#import MessageWindow
import GUICommonWindows
from GUISAVE import *
from GUICommonWindows import *

###################################################
GameOptionsWindow = None
PortraitWindow = None
OldPortraitWindow = None

VideoOptionsWindow = None
AudioOptionsWindow = None
GameplayOptionsWindow = None
FeedbackOptionsWindow = None
AutopauseOptionsWindow = None
LoadMsgWindow = None
QuitMsgWindow = None

###################################################
def CloseOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow
	
	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVisible (0,1)
	GemRB.UnhideGUI ()
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	OldPortraitWindow = None
	SetSelectionChangeHandler (None)
	return

###################################################
def OpenOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow

	if CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow ()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIOPT", 640, 480)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow (0)
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0)
	GemRB.SetWindowFrame (OptionsWindow)

	# Return to Game
	Button = GemRB.GetControl (Window, 11)
	GemRB.SetText (Window, Button, 10308)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")	

	# Quit Game
	Button = GemRB.GetControl (Window, 10)
	GemRB.SetText (Window, Button, 13731)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")

	# Load Game
	Button = GemRB.GetControl (Window, 5)
	GemRB.SetText (Window, Button, 13729)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")

	# Save Game
	Button = GemRB.GetControl (Window, 6)
	GemRB.SetText (Window, Button, 13730)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenSaveMsgWindow")

	# Video Options
	Button = GemRB.GetControl (Window, 7)
	GemRB.SetText (Window, Button, 17162)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")

	# Audio Options
	Button = GemRB.GetControl (Window, 8)
	GemRB.SetText (Window, Button, 17164)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")

	# Gameplay Options
	Button = GemRB.GetControl (Window, 9)
	GemRB.SetText (Window, Button, 17165)
	GemRB.SetEvent (Window, Button, IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")

	# game version, e.g. v1.1.0000
	Label = GemRB.GetControl (Window, 0x1000000b)
	GemRB.SetText (Window, Label, GEMRB_VERSION)
	
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

	
###################################################

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global VideoOptionsWindow, VideoHelpText

	if VideoOptionsWindow:
		GemRB.UnloadWindow (VideoOptionsWindow)
		VideoOptionsWindow = None
		return

	
	VideoOptionsWindow = Window = GemRB.LoadWindow (1)

	VideoHelpText = OptHelpText ('VideoOptions', Window, 9, 31052)

	OptDone ('VideoOptions', Window, 7)
	OptCancel ('VideoOptions', Window, 8)

	OptSlider ('Brightness', Window, 1, 10, 31234)
	OptSlider ('Contrast', Window, 2, 11, 31429)

	OptCheckbox ('SoftwareBlitting', Window, 6, 15, 30898)
	OptCheckbox ('SoftwareMirroring', Window, 4, 13, 30896)
	OptCheckbox ('SoftwareTransparency', Window, 5, 14, 30897)

	GemRB.SetVisible (Window, 1)
	

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

	if AudioOptionsWindow:
		GemRB.UnloadWindow (AudioOptionsWindow)
		AudioOptionsWindow = None
		return

	
	AudioOptionsWindow = Window = GemRB.LoadWindow (5)

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


	GemRB.SetVisible (AudioOptionsWindow, 1)
	

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

	if GameplayOptionsWindow:
		if FeedbackOptionsWindow: OpenFeedbackOptionsWindow ()
		if AutopauseOptionsWindow: OpenAutopauseOptionsWindow ()

		GemRB.UnloadWindow (GameplayOptionsWindow)
		GameplayOptionsWindow = None
		return

	
	GameplayOptionsWindow = Window = GemRB.LoadWindow (6)
	

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

	GemRB.SetVisible (Window, 1)


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
	
	if FeedbackOptionsWindow:
		GemRB.UnloadWindow (FeedbackOptionsWindow)
		FeedbackOptionsWindow = None
		return

	
	FeedbackOptionsWindow = Window = GemRB.LoadWindow (8)

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


	GemRB.SetVisible (FeedbackOptionsWindow, 1)
	

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
	
	if AutopauseOptionsWindow:
		GemRB.UnloadWindow (AutopauseOptionsWindow)
		AutopauseOptionsWindow = None
		return

	
	AutopauseOptionsWindow = Window = GemRB.LoadWindow (9)

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

	GemRB.SetVisible (AutopauseOptionsWindow, 1)
	

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

def OpenSaveMsgWindow ():
	#CloseOptionsWindow ()
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
	return

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def LoadGamePress ():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return

#save game AND quit
def SaveGamePress():
	#CloseOptionsWindow ()
	OpenSaveWindow ()	
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

def QuitGamePress():
	GemRB.QuitGame ()
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

	# The game has not been saved ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 16456) 

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

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
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)
	#GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_ENTER_BUTTON, "DisplayHelp" + name)

	return slider


def OptCheckbox (name, window, button_id, label_id, label_strref):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetButtonState (window, button, IE_GUI_BUTTON_SELECTED)
	GemRB.SetEvent (window, button, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetText (window, label, label_strref)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)
	#GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_ENTER_BUTTON, "DisplayHelp" + name)

	return button

def OptButton (name, window, button_id, label_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)	
	GemRB.SetEvent (window, button, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetText (window, label, label_strref)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)
	#GemRB.SetEvent (window, label, IE_GUI_MOUSE_OVER_BUTTON, "DisplayHelp" + name)
	GemRB.SetEvent (window, label, IE_GUI_MOUSE_ENTER_BUTTON, "DisplayHelp" + name)

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

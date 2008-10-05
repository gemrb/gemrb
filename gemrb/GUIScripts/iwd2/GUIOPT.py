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
# $Id$

# GUIOPT.py - scripts to control options windows mostly from GUIOPT winpack
# Ingame options

###################################################
import GemRB
import GUICommonWindows
from GUIDefines import *
from GUISAVE import *
from GUICommon import CloseOtherWindow
from GUICommonWindows import *

###################################################
GameOptionsWindow = None
VideoOptionsWindow = None
AudioOptionsWindow = None
GameplayOptionsWindow = None
FeedbackOptionsWindow = None
AutopauseOptionsWindow = None
LoadMsgWindow = None
SaveMsgWindow = None
QuitMsgWindow = None
MovieWindow = None
KeysWindow = None
OptionsWindow = None
PortraitWindow = None
OldPortraitWindow = None
OldOptionsWindow = None

###################################################
def CloseOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if GameOptionsWindow == None:
		return

	GemRB.UnloadWindow (GameOptionsWindow)
	GemRB.UnloadWindow (OptionsWindow)
	GemRB.UnloadWindow (PortraitWindow)

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GemRB.SetVisible (0,1)
	GemRB.UnhideGUI ()
	GUICommonWindows.OptionsWindow = OldOptionsWindow
	OldOptionsWindow = None
	GUICommonWindows.PortraitWindow = OldPortraitWindow
	OldPortraitWindow = None
	return

###################################################
def OpenOptionsWindow ():
	global GameOptionsWindow, OptionsWindow, PortraitWindow
	global OldPortraitWindow, OldOptionsWindow

	if CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow ()
		return

	GemRB.HideGUI ()
	GemRB.SetVisible (0,0)

	GemRB.LoadWindowPack ("GUIOPT", 800, 600)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	SetupMenuWindowControls (OptionsWindow, 0, "OpenOptionsWindow")
	GemRB.SetWindowFrame (Window)

	LoadButton = GemRB.GetControl (Window, 5)
	SaveButton = GemRB.GetControl (Window, 6)
	QuitButton = GemRB.GetControl (Window, 10)
	GraphicsButton = GemRB.GetControl (Window, 7)
	SoundButton = GemRB.GetControl (Window, 8)
	GamePlayButton = GemRB.GetControl (Window, 9)
	MoviesButton = GemRB.GetControl (Window, 14)
	KeyboardButton = GemRB.GetControl (Window, 13)
	ReturnButton = GemRB.GetControl (Window, 11)

	GemRB.SetText (Window, LoadButton, 13729)
	GemRB.SetEvent (Window, LoadButton, IE_GUI_BUTTON_ON_PRESS, "OpenLoadMsgWindow")
	GemRB.SetText (Window, SaveButton, 13730)
	GemRB.SetEvent (Window, SaveButton, IE_GUI_BUTTON_ON_PRESS, "OpenSaveMsgWindow")
	GemRB.SetText (Window, QuitButton, 13731)
	GemRB.SetEvent (Window, QuitButton, IE_GUI_BUTTON_ON_PRESS, "OpenQuitMsgWindow")
	GemRB.SetText (Window, GraphicsButton, 17162)
	GemRB.SetEvent (Window, GraphicsButton, IE_GUI_BUTTON_ON_PRESS, "OpenVideoOptionsWindow")
	GemRB.SetText (Window, SoundButton, 17164)
	GemRB.SetEvent (Window, SoundButton, IE_GUI_BUTTON_ON_PRESS, "OpenAudioOptionsWindow")
	GemRB.SetText (Window, GamePlayButton, 17165)
	GemRB.SetEvent (Window, GamePlayButton, IE_GUI_BUTTON_ON_PRESS, "OpenGameplayOptionsWindow")
	GemRB.SetText (Window, MoviesButton, 15415)
	GemRB.SetEvent (Window, MoviesButton, IE_GUI_BUTTON_ON_PRESS, "OpenMovieWindow")
	GemRB.SetText (Window, KeyboardButton, 33468)
	GemRB.SetEvent (Window, KeyboardButton, IE_GUI_BUTTON_ON_PRESS, "OpenKeyboardWindow")
	GemRB.SetText (Window, ReturnButton, 10308)
	GemRB.SetEvent (Window, ReturnButton, IE_GUI_BUTTON_ON_PRESS, "OpenOptionsWindow")

	VersionLabel = GemRB.GetControl (Window, 0x1000000B)
	GemRB.SetText (Window, VersionLabel, GEMRB_VERSION)

	GemRB.SetVisible (Window, 1)
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def CloseVideoOptionsWindow ():
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (6)

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

	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)
	return

def DisplayHelpFullScreen ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17205)

def DisplayHelpBrightness ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17203)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)
	return

def DisplayHelpContrast ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)
	return

def DisplayHelpSoftMirrBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18004)

def DisplayHelpSoftTransBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18006)

def DisplayHelpSoftStandBlt ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18007)

def DisplayHelpTransShadow ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 20620)


###################################################

def CloseAudioOptionsWindow ():
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (7)

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
	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)


def DisplayHelpAmbientVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18008)
	GemRB.UpdateAmbientsVolume()

def DisplayHelpSoundFXVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18009)

def DisplayHelpVoiceVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18010)

def DisplayHelpMusicVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18011)
	GemRB.UpdateMusicVolume()

def DisplayHelpMovieVolume ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18012)

def DisplayHelpCreativeEAX ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18022)


###################################################

def CloseGameplayOptionsWindow ():
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	#gameplayoptions
	GameOptionsWindow = Window = GemRB.LoadWindow (8)


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
	OptCheckbox ('MaxHitpoints', Window, 50, 49, 'Maximum HP', 1)

	OptButton ('FeedbackOptions', Window, 5, 17163)
	OptButton ('AutopauseOptions', Window, 6, 17166)

	GemRB.ShowModal (GameOptionsWindow, MODAL_SHADOW_GRAY)
	return

def DisplayHelpTooltipDelay ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") )

def DisplayHelpMouseScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

def DisplayHelpKeyboardScrollingSpeed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18019)

def DisplayHelpDifficulty ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18020)

def DisplayHelpDitherAlways ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18021)

def DisplayHelpGore ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18023)

def DisplayHelpInfravision ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 11797)

def DisplayHelpWeather ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 20619)

def DisplayHelpRestUntilHealed ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 2242)

def DisplayMaxHitpoints ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 15136)

###################################################

def CloseFeedbackOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()


def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	#feedback
	GameOptionsWindow = Window = GemRB.LoadWindow (9)

	HelpTextArea = OptHelpText ('FeedbackOptions', Window, 28, 18043)

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
	return

def DisplayHelpMarkerFeedback ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18024)

def DisplayHelpLocatorFeedback ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18025)

def DisplayHelpToHitRolls ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18026)

def DisplayHelpCombatInfo ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18027)

def DisplayHelpActions ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18028)

def DisplayHelpStates ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18029)

def DisplayHelpSelection ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18030)

def DisplayHelpMiscellaneous ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18031)


###################################################

def CloseAutopauseOptionsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()
	return

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (10)

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
	OptCheckbox ('SpellCast', Window, 34, 30, 'Auto Pause State', 256)
	OptCheckbox ('TrapFound', Window, 31, 33, 'Auto Pause State', 512)
	OptCheckbox ('CenterOnActor', Window, 31, 33, 'Auto Pause Center', 1)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def DisplayHelpCharacterHit ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18032)

def DisplayHelpCharacterInjured ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18033)

def DisplayHelpCharacterDead ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18034)

def DisplayHelpCharacterAttacked ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18035)

def DisplayHelpWeaponUnusable ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18036)

def DisplayHelpTargetGone ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18037)

def DisplayHelpEndOfRound ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 10640)

def DisplayHelpEnemySighted ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 23514)

def DisplayHelpSpellCast ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 58171)

def DisplayHelpTrapFound ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 31872)

def DisplayHelpCenterOnActor ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 10571)

###################################################

def CloseCharacterSoundsWindow ():
	global GameOptionsWindow

	GemRB.UnloadWindow (GameOptionsWindow)
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()
	return

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		GemRB.UnloadWindow (GameOptionsWindow)
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (12)

	HelpTextArea = OptHelpText ('CharacterSounds', Window, 16, 18041)

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

def DisplayHelpSubtitles ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18015)

def DisplayHelpAttackSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18013)

def DisplayHelpFootsteps ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18014)

def DisplayHelpCommandSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 18016)

def DisplayHelpSelectionSounds ():
	GemRB.SetText (GameOptionsWindow, HelpTextArea, 11352)

###################################################

def CloseMovieWindow ():
	GemRB.UnloadWindow (MovieWindow)
	return

def MoviePlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = GemRB.GetTableRowName(MoviesTable, i)
				GemRB.PlayMovie(s, 1)
				GemRB.InvalidateWindow(MovieWindow)
				return
			s = s - 1
	return

def MovieCreditsPress():
	GemRB.PlayMovie("CREDITS")
	GemRB.InvalidateWindow(MovieWindow)
	return

def OpenMovieWindow ():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	MovieWindow = Window = GemRB.LoadWindow(2)
	GemRB.SetWindowFrame (Window)
	#reloading the guiopt windowpack
	GemRB.LoadWindowPack ("GUIOPT", 800, 600)
	TextAreaControl = GemRB.GetControl(Window, 0)
	GemRB.SetTextAreaFlags(Window, TextAreaControl,IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = GemRB.GetControl(Window, 2)
	CreditsButton = GemRB.GetControl(Window, 3)
	DoneButton = GemRB.GetControl(Window, 4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, GemRB.GetTableRowCount(MoviesTable) ):
		t = GemRB.GetTableRowName(MoviesTable, i)
		if GemRB.GetVar(t)==1:
			s = GemRB.GetTableValue(MoviesTable, i, 0)
			GemRB.TextAreaAppend(Window, TextAreaControl, s,-1)
	GemRB.SetVarAssoc(Window, TextAreaControl, "MovieIndex",0)
	GemRB.SetText(Window, PlayButton, 17318)
	GemRB.SetText(Window, CreditsButton, 15591)
	GemRB.SetText(Window, DoneButton, 11973)
	GemRB.SetEvent(Window, PlayButton, IE_GUI_BUTTON_ON_PRESS, "MoviePlayPress")
	GemRB.SetEvent(Window, CreditsButton, IE_GUI_BUTTON_ON_PRESS, "MovieCreditsPress")
	GemRB.SetEvent(Window, DoneButton, IE_GUI_BUTTON_ON_PRESS, "CloseMovieWindow")
	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

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
	return

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

def LoadGamePress ():
	global LoadMsgWindow

	GemRB.UnloadWindow (LoadMsgWindow)
	LoadMsgWindow = None
	GemRB.QuitGame ()
	OpenOptionsWindow()
	GemRB.SetNextScript ("GUILOAD")
	return

#save game AND quit
def SaveGamePress ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	OpenSaveWindow ()
	return

def QuitGamePress ():
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

	# The game has not been saved ....
	Text = GemRB.GetControl (Window, 3)
	GemRB.SetText (Window, Text, 16456)

	GemRB.ShowModal (Window, MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	GemRB.UnloadWindow (QuitMsgWindow)
	QuitMsgWindow = None
	GemRB.SetVisible (GameOptionsWindow, 1)
	GemRB.SetVisible (OptionsWindow, 1)
	GemRB.SetVisible (PortraitWindow, 1)
	return

###################################################
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
	return slider

def OptRadio (name, window, button_id, label_id, variable, value):
	"""Standard radio button for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)
	GemRB.SetButtonSprites(window, button, "GBTNOPT4", 0, 0, 1, 2, 3)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)

	return button

def OptCheckbox (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = GemRB.GetControl (window, button_id)
	GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_OR)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "DisplayHelp" + name)
	GemRB.SetVarAssoc (window, button, variable, value)
	GemRB.SetButtonSprites(window, button, "GBTNOPT4", 0, 0, 1, 2, 3)

	label = GemRB.GetControl (window, label_id)
	GemRB.SetButtonFlags (window, label, IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	GemRB.SetButtonState (window, label, IE_GUI_BUTTON_LOCKED)

	return button

def OptButton (name, window, button_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = GemRB.GetControl (window, button_id)
	GemRB.SetEvent (window, button, IE_GUI_BUTTON_ON_PRESS, "Open%sWindow" %name)
	GemRB.SetText (window, button, label_strref)

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

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
# Ingame options

###################################################
import GemRB
import GUICommon
import GUICommonWindows
import GUISAVE
from GUIDefines import *

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

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	if OptionsWindow:
		OptionsWindow.Unload ()
	if PortraitWindow:
		PortraitWindow.Unload ()

	GameOptionsWindow = None
	GemRB.SetVar ("OtherWindow", -1)
	GUICommon.GameWindow.SetVisible(WINDOW_VISIBLE)
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

	if GUICommon.CloseOtherWindow (OpenOptionsWindow):
		CloseOptionsWindow ()
		return

	GemRB.HideGUI ()
	GUICommon.GameWindow.SetVisible(WINDOW_INVISIBLE)

	GemRB.LoadWindowPack ("GUIOPT", 800, 600)
	GameOptionsWindow = Window = GemRB.LoadWindow (2)
	GemRB.SetVar ("OtherWindow", GameOptionsWindow.ID)
	#saving the original portrait window
	OldPortraitWindow = GUICommonWindows.PortraitWindow
	PortraitWindow = GUICommonWindows.OpenPortraitWindow ()
	OldOptionsWindow = GUICommonWindows.OptionsWindow
	OptionsWindow = GemRB.LoadWindow (0)
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 0, OpenOptionsWindow)
	Window.SetFrame ()

	LoadButton = Window.GetControl (5)
	SaveButton = Window.GetControl (6)
	QuitButton = Window.GetControl (10)
	GraphicsButton = Window.GetControl (7)
	SoundButton = Window.GetControl (8)
	GamePlayButton = Window.GetControl (9)
	MoviesButton = Window.GetControl (14)
	KeyboardButton = Window.GetControl (13)
	ReturnButton = Window.GetControl (11)

	LoadButton.SetText (13729)
	LoadButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenLoadMsgWindow)
	SaveButton.SetText (13730)
	SaveButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSaveMsgWindow)
	QuitButton.SetText (13731)
	QuitButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenQuitMsgWindow)
	GraphicsButton.SetText (17162)
	GraphicsButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenVideoOptionsWindow)
	SoundButton.SetText (17164)
	SoundButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenAudioOptionsWindow)
	GamePlayButton.SetText (17165)
	GamePlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenGameplayOptionsWindow)
	MoviesButton.SetText (15415)
	MoviesButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenMovieWindow)
	KeyboardButton.SetText (33468)
	KeyboardButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: OpenKeyboardWindow
	ReturnButton.SetText (10308)
	ReturnButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenOptionsWindow)
	ReturnButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	VersionLabel = Window.GetControl (0x1000000B)
	VersionLabel.SetText (GEMRB_VERSION)

	Window.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
	return

def CloseVideoOptionsWindow ():
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenVideoOptionsWindow ():
	"""Open video options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
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

	GameOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpFullScreen ():
	HelpTextArea.SetText (18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	HelpTextArea.SetText (17205)

def DisplayHelpBrightness ():
	HelpTextArea.SetText (17203)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)
	return

def DisplayHelpContrast ():
	HelpTextArea.SetText (17204)
	GemRB.SetGamma (GemRB.GetVar("Brightness Correction"),GemRB.GetVar("Gamma Correction")/2)
	return

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
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenAudioOptionsWindow ():
	"""Open audio options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
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
	GameOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)


def DisplayHelpAmbientVolume ():
	HelpTextArea.SetText (18008)
	GemRB.UpdateAmbientsVolume()

def DisplayHelpSoundFXVolume ():
	HelpTextArea.SetText (18009)

def DisplayHelpVoiceVolume ():
	HelpTextArea.SetText (18010)

def DisplayHelpMusicVolume ():
	HelpTextArea.SetText (18011)
	GemRB.UpdateMusicVolume()

def DisplayHelpMovieVolume ():
	HelpTextArea.SetText (18012)

def DisplayHelpCreativeEAX ():
	HelpTextArea.SetText (18022)


###################################################

def CloseGameplayOptionsWindow ():
	OpenOptionsWindow ()
	OpenOptionsWindow ()

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
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

	GameOptionsWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

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

def DisplayMaxHitpoints ():
	HelpTextArea.SetText (15136)

###################################################

def CloseFeedbackOptionsWindow ():
	global GameOptionsWindow

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()


def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
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

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpMarkerFeedback ():
	HelpTextArea.SetText (18024)

def DisplayHelpLocatorFeedback ():
	HelpTextArea.SetText (18025)

def DisplayHelpToHitRolls ():
	HelpTextArea.SetText (18026)

def DisplayHelpCombatInfo ():
	HelpTextArea.SetText (18027)

def DisplayHelpActions ():
	HelpTextArea.SetText (18028)

def DisplayHelpStates ():
	HelpTextArea.SetText (18029)

def DisplayHelpSelection ():
	HelpTextArea.SetText (18030)

def DisplayHelpMiscellaneous ():
	HelpTextArea.SetText (18031)


###################################################

def CloseAutopauseOptionsWindow ():
	global GameOptionsWindow

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()
	return

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
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

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

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

def DisplayHelpSpellCast ():
	HelpTextArea.SetText (58171)

def DisplayHelpTrapFound ():
	HelpTextArea.SetText (31872)

def DisplayHelpCenterOnActor ():
	HelpTextArea.SetText (10571)

###################################################

def CloseCharacterSoundsWindow ():
	global GameOptionsWindow

	if GameOptionsWindow:
		GameOptionsWindow.Unload ()
	GameOptionsWindow = None
	OpenGameplayOptionsWindow ()
	return

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""
	global GameOptionsWindow, HelpTextArea

	if GameOptionsWindow:
		if GameOptionsWindow:
			GameOptionsWindow.Unload ()
		GameOptionsWindow = None

	GameOptionsWindow = Window = GemRB.LoadWindow (12)

	HelpTextArea = OptHelpText ('CharacterSounds', Window, 16, 18041)

	OptDone ('CharacterSounds', Window, 24)
	OptCancel ('CharacterSounds', Window, 25)

	OptCheckbox ('Subtitles', Window, 5, 20, 'Subtitles', 1)
	OptCheckbox ('AttackSounds', Window, 6, 18, 'Attack Sounds', 1)
	OptCheckbox ('Footsteps', Window, 7, 19, 'Footsteps', 1)
	OptRadio ('CommandSounds', Window, 8, 21, 'Command Sounds Frequency', 2)
	OptRadio ('CommandSounds', Window, 9, 21, 'Command Sounds Frequency', 1)
	OptRadio ('CommandSounds', Window, 10, 21, 'Command Sounds Frequency', 0)
	OptRadio ('SelectionSounds', Window, 58, 57, 'Selection Sounds Frequency', 2)
	OptRadio ('SelectionSounds', Window, 59, 57, 'Selection Sounds Frequency', 1)
	OptRadio ('SelectionSounds', Window, 60, 57, 'Selection Sounds Frequency', 0)

	Window.ShowModal (MODAL_SHADOW_GRAY)

def DisplayHelpSubtitles ():
	HelpTextArea.SetText (18015)

def DisplayHelpAttackSounds ():
	HelpTextArea.SetText (18013)

def DisplayHelpFootsteps ():
	HelpTextArea.SetText (18014)

def DisplayHelpCommandSounds ():
	HelpTextArea.SetText (18016)

def DisplayHelpSelectionSounds ():
	HelpTextArea.SetText (11352)

def DisplayHelpMaxHitpoints ():
	#TODO
	return

###################################################

def CloseMovieWindow ():
	if MovieWindow:
		MovieWindow.Unload ()
	return

def MoviePlayPress():
	s = GemRB.GetVar("MovieIndex")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			if s==0:
				s = MoviesTable.GetRowName(i)
				GemRB.PlayMovie(s, 1)
				MovieWindow.Invalidate()
				return
			s = s - 1
	return

def MovieCreditsPress():
	GemRB.PlayMovie("CREDITS")
	MovieWindow.Invalidate()
	return

def OpenMovieWindow ():
	global MovieWindow, TextAreaControl, MoviesTable

	GemRB.LoadWindowPack("GUIMOVIE", 800, 600)
	MovieWindow = Window = GemRB.LoadWindow(2)
	Window.SetFrame ()
	#reloading the guiopt windowpack
	GemRB.LoadWindowPack ("GUIOPT", 800, 600)
	TextAreaControl = Window.GetControl(0)
	TextAreaControl.SetFlags(IE_GUI_TEXTAREA_SELECTABLE)
	PlayButton = Window.GetControl(2)
	CreditsButton = Window.GetControl(3)
	DoneButton = Window.GetControl(4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	for i in range(0, MoviesTable.GetRowCount() ):
		t = MoviesTable.GetRowName(i)
		if GemRB.GetVar(t)==1:
			s = MoviesTable.GetValue(i, 0)
			TextAreaControl.Append(s,-1)
	TextAreaControl.SetVarAssoc("MovieIndex",0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MoviePlayPress)
	CreditsButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, MovieCreditsPress)
	DoneButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, CloseMovieWindow)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def OpenSaveMsgWindow ():
	GemRB.SetVar("QuitAfterSave",0)
	GUISAVE.OpenSaveWindow ()
	#save the game without quitting
	return

###################################################

def OpenLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		return

	LoadMsgWindow = Window = GemRB.LoadWindow (4)

	# Load
	Button = Window.GetControl (0)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadGamePress)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseLoadMsgWindow)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (19531)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseLoadMsgWindow ():
	global LoadMsgWindow

	if LoadMsgWindow:
		LoadMsgWindow.Unload ()
	LoadMsgWindow = None
	GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
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

#save game AND quit
def SaveGamePress ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	OpenOptionsWindow()
	GUISAVE.OpenSaveWindow ()
	return

def QuitGamePress ():
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

	QuitMsgWindow = Window = GemRB.LoadWindow (5)

	# Save
	Button = Window.GetControl (0)
	Button.SetText (15589)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SaveGamePress)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitGamePress)

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseQuitMsgWindow)

	# The game has not been saved ....
	Text = Window.GetControl (3)
	Text.SetText (16456)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Unload ()
	QuitMsgWindow = None
	GameOptionsWindow.SetVisible (WINDOW_VISIBLE)
	OptionsWindow.SetVisible (WINDOW_VISIBLE)
	PortraitWindow.SetVisible (WINDOW_VISIBLE)
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
	slider = window.GetControl (slider_id)
	slider.SetVarAssoc (variable, value)
	slider.SetEvent (IE_GUI_SLIDER_ON_CHANGE, eval("DisplayHelp" + name))
	return slider

def OptRadio (name, window, button_id, label_id, variable, value):
	"""Standard radio button for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON, OP_OR)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("DisplayHelp" + name))
	button.SetVarAssoc (variable, value)
	button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	label = window.GetControl (label_id)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)

	return button

def OptCheckbox (name, window, button_id, label_id, variable, value):
	"""Standard checkbox for option windows"""

	button = window.GetControl (button_id)
	button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("DisplayHelp" + name))
	button.SetVarAssoc (variable, value)
	button.SetSprites("GBTNOPT4", 0, 0, 1, 2, 3)

	label = window.GetControl (label_id)
	label.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
	label.SetState (IE_GUI_BUTTON_LOCKED)

	return button

def OptButton (name, window, button_id, label_strref):
	"""Standard subwindow button for option windows"""
	button = window.GetControl (button_id)
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("Open%sWindow" %name))
	button.SetText (label_strref)

def OptDone (name, window, button_id):
	"""Standard `Done' button for option windows"""
	button = window.GetControl (button_id)
	button.SetText (11973) # Done
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("Close%sWindow" %name))

def OptCancel (name, window, button_id):
	"""Standard `Cancel' button for option windows"""
	button = window.GetControl (button_id)
	button.SetText (13727) # Cancel
	button.SetEvent (IE_GUI_BUTTON_ON_PRESS, eval("Close%sWindow" %name))

def OptHelpText (name, window, text_id, text_strref):
	"""Standard textarea with context help for option windows"""
	text = window.GetControl (text_id)
	text.SetText (text_strref)
	return text

###################################################
# End of file GUIOPT.py

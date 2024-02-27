# GemRB - Infinity Engine Emulator
# Copyright (C) 2012 The GemRB Project
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#

# GUIOPT.py - scripts to control options windows mostly from the GUIOPT winpack:
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
import GameCheck
import GemRB
import GUICommonWindows
import GUISAVE
import GUIOPTControls
from GUIDefines import *

###################################################

QuitMsgWindow = None

###################################################
def InitOptionsWindow (Window):
	"""Open main options window"""

	Container.CloseContainerWindow ()

	# Return to Game
	Button = Window.GetControl (11)
	Button.SetText (10308)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	# Quit Game
	Button = Window.GetControl (10)
	Button.SetText (13731)
	Button.OnPress (OpenQuitMsgWindow)

	# Load Game
	Button = Window.GetControl (5)
	Button.SetText (13729)
	Button.OnPress (LoadGamePress)

	# Save Game
	Button = Window.GetControl (6)
	Button.SetText (13730)
	Button.OnPress (OpenSaveMsgWindow)

	# Video Options
	Button = Window.GetControl (7)
	Button.SetText (17162)
	Button.OnPress (OpenVideoOptionsWindow)

	# Audio Options
	Button = Window.GetControl (8)
	Button.SetText (17164)
	Button.OnPress (OpenAudioOptionsWindow)

	# Gameplay Options
	Button = Window.GetControl (9)
	Button.SetText (17165)
	Button.OnPress (OpenGameplayOptionsWindow)

	# game version, e.g. v1.1.0000
	VersionLabel = Window.GetControl (0x1000000b)
	VersionLabel.SetText (GemRB.Version)

	if GameCheck.IsIWD2():
		# Keyboard shortcuts
		KeyboardButton = Window.GetControl (13)
		KeyboardButton.SetText (33468)
		KeyboardButton.OnPress (OpenHotkeyOptionsWindow)

		# Movies
		MoviesButton = Window.GetControl (14)
		MoviesButton.SetText (15415)
		MoviesButton.OnPress (OpenMovieWindow)

	return

ToggleOptionsWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIOPT", GUICommonWindows.ToggleWindow, InitOptionsWindow, None, GUICommonWindows.DefaultWinPos, True)
OpenOptionsWindow = GUICommonWindows.CreateTopWinLoader(2, "GUIOPT", GUICommonWindows.OpenWindowOnce, InitOptionsWindow, None, GUICommonWindows.DefaultWinPos, True)

def TrySavingConfiguration():
	if not GemRB.SaveConfig():
		print("ARGH, could not write config to disk!!")

###################################################

# work around radiobutton preselection issues
# after the generation of the controls we have to re-adjust the visible and
# internal state, i.e. adjust button state and dictionary entry
def PreselectRadioGroup (variable, value, buttonIds, window):
	for i in buttonIds:
		Button = window.GetControl (i)
		if (Button.Value == value):
			Button.SetState (IE_GUI_BUTTON_SELECTED)
		else:
			Button.SetState (IE_GUI_BUTTON_ENABLED)
	GemRB.SetVar(variable, value)

###################################################

def CloseVideoOptionsWindow ():
	GemRB.GetView("SUB_WIN", 0).Close()
	TrySavingConfiguration()

def OpenVideoOptionsWindow ():
	"""Open video options window"""

	Window = GemRB.LoadWindow (6, "GUIOPT")
	Window.AddAlias("SUB_WIN", 0)
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	GUIOPTControls.OptHelpText (33, 18038)

	GUIOPTControls.OptDone (CloseVideoOptionsWindow, 21)
	GUIOPTControls.OptCancel (CloseVideoOptionsWindow, 32)

	GUIOPTControls.OptSlider (17203, 3, 35, 17129, 'Brightness Correction', DisplayHelpBrightness, 4)
	GUIOPTControls.OptSlider (17204, 22, 36, 17128, 'Gamma Correction', DisplayHelpContrast)

	# Radiobuttons need special care...
	Variable = 'BitsPerPixel'
	Value = GemRB.GetVar(Variable)
	ButtonIds = [5, 6, 7]

	GUIOPTControls.OptRadio (DisplayHelpBPP, ButtonIds[0], 37, Variable, 16, None, 17205, 18038)
	GUIOPTControls.OptRadio (DisplayHelpBPP, ButtonIds[1], 37, Variable, 24, None, 17205, 18038)
	GUIOPTControls.OptRadio (DisplayHelpBPP, ButtonIds[2], 37, Variable, 32, None, 17205, 18038)
	PreselectRadioGroup (Variable, Value, ButtonIds, Window)

	GUIOPTControls.OptCheckbox (18000, 9, 38, 17131, 'Full Screen', DisplayHelpFullScreen)

	GUIOPTControls.OptCheckbox (20620, 51, 50, 20617, 'Translucent Shadows')
	GUIOPTControls.OptCheckbox (15135, 40, 44, 17134, 'SoftMirrorBlt')
	GUIOPTControls.OptCheckbox (18006, 41, 46, 17136, 'SoftSrcKeyBlt') # software standard blit
	GUIOPTControls.OptCheckbox (18007, 42, 48, 17135, 'SoftBltFast') # software transparent blit

	# maybe not present in original iwd, but definitely in how
	if GameCheck.IsIWD1 () or GameCheck.IsIWD2 ():
		GUIOPTControls.OptCheckbox (15141, 56, 52, 14447, 'TranslucentBlt')
		GUIOPTControls.OptCheckbox (18004, 57, 54, 14578, 'StaticAnims')

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpFullScreen ():
	GemRB.GetView ("OPTHELP").SetText (18000)
	GemRB.SetFullScreen (GemRB.GetVar("Full Screen"))

def DisplayHelpBPP ():
	GemRB.GetView ("OPTHELP").SetText (17205)

def DisplayHelpBrightness ():
	GemRB.GetView ("OPTHELP").SetText (17203)
	SetGfxCorrection ()

def DisplayHelpContrast ():
	GemRB.GetView ("OPTHELP").SetText (17204)
	SetGfxCorrection ()

# different games have different slider ranges, but the engine wants:
# gamma: 0-5
# brightness: 0-40
def SetGfxCorrection ():
	Brightness = GemRB.GetVar("Brightness Correction")
	Gamma = GemRB.GetVar("Gamma Correction")
	if GameCheck.IsHOW() or GameCheck.IsIWD2(): # 10/11 ticks
		Gamma //= 2

	GemRB.SetGamma (Brightness, Gamma)

###################################################

def CloseAudioOptionsWindow ():
	GemRB.GetView("SUB_WIN", 0).Close()
	TrySavingConfiguration()

def OpenAudioOptionsWindow ():
	"""Open audio options window"""

	Window = GemRB.LoadWindow (7, "GUIOPT")
	Window.AddAlias("SUB_WIN", 0)
	Window.SetFlags (WF_BORDERLESS, OP_OR)
	GUIOPTControls.OptHelpText (14, 18040)

	GUIOPTControls.OptDone (CloseAudioOptionsWindow, 24)
	GUIOPTControls.OptCancel (CloseAudioOptionsWindow, 25)
	GUIOPTControls.OptButton (OpenCharacterSoundsWindow, 13, 17778)

	GUIOPTControls.OptSlider (18008, 1, 16, 16514, 'Volume Ambients', DisplayHelpAmbientVolume, 10)
	GUIOPTControls.OptSlider (18009, 2, 17, 16515, 'Volume SFX', None, 10)
	GUIOPTControls.OptSlider (18010, 3, 18, 16512, 'Volume Voices', None, 10)
	GUIOPTControls.OptSlider (18011, 4, 19, 16511, 'Volume Music', DisplayHelpMusicVolume, 10)
	GUIOPTControls.OptSlider (18012, 22, 20, 16546, 'Volume Movie', None, 10)

	GUIOPTControls.OptCheckbox (18022, 26, 28, 20689, 'Environmental Audio')

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpAmbientVolume ():
	GemRB.GetView ("OPTHELP").SetText (18008)
	GemRB.UpdateAmbientsVolume ()

def DisplayHelpMusicVolume ():
	GemRB.GetView ("OPTHELP").SetText (18011)
	GemRB.UpdateMusicVolume ()

###################################################

def OpenCharacterSoundsWindow ():
	"""Open character sounds window"""

	Window = GemRB.LoadWindow (12, "GUIOPT")
	Window.AddAlias("SUB_WIN", 1)
	Window.SetFlags (WF_BORDERLESS, OP_OR)
	GUIOPTControls.OptHelpText (16, 18041)

	GUIOPTControls.OptDone (Window.Close, 24)
	GUIOPTControls.OptCancel (Window.Close, 25)

	GUIOPTControls.OptCheckbox (18015, 5, 20, 17138, 'Subtitles')
	GUIOPTControls.OptCheckbox (18013, 6, 18, 17139, 'Attack Sounds')
	GUIOPTControls.OptCheckbox (18014, 7, 19, 17140, 'Footsteps')

	# Radiobuttons need special care...
	Variable = 'Command Sounds Frequency'
	Value = GemRB.GetVar(Variable)
	ButtonIds = [8, 9, 10]

	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, ButtonIds[0], 21, Variable, 3)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, ButtonIds[1], 21, Variable, 2)
	GUIOPTControls.OptRadio (DisplayHelpCommandSounds, ButtonIds[2], 21, Variable, 1)
	PreselectRadioGroup (Variable, Value, ButtonIds, Window)

	Variable = 'Selection Sounds Frequency'
	Value = GemRB.GetVar(Variable)
	ButtonIds = [58, 59, 60]

	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, ButtonIds[0], 57, Variable, 3)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, ButtonIds[1], 57, Variable, 2)
	GUIOPTControls.OptRadio (DisplayHelpSelectionSounds, ButtonIds[2], 57, Variable, 1)
	PreselectRadioGroup (Variable, Value, ButtonIds, Window)

	Window.ShowModal (MODAL_SHADOW_GRAY)

def DisplayHelpCommandSounds ():
	GemRB.GetView ("OPTHELP").SetText (18016)

def DisplayHelpSelectionSounds ():
	GemRB.GetView ("OPTHELP").SetText (11352)

###################################################

def CloseGameplayOptionsWindow ():
	# FIXME: don't need to do anything, since we save stuff immediately
	# ergo canceling does not work
	GemRB.GetView("SUB_WIN", 0).Close()
	TrySavingConfiguration()

def OpenGameplayOptionsWindow ():
	"""Open gameplay options window"""

	#gameplayoptions
	Window = GemRB.LoadWindow (8, "GUIOPT")
	Window.AddAlias("SUB_WIN", 0)
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	GUIOPTControls.OptHelpText (40, 18042)

	GUIOPTControls.OptDone (CloseGameplayOptionsWindow, 7)
	GUIOPTControls.OptCancel (CloseGameplayOptionsWindow, 20)

	GUIOPTControls.OptSlider (18017, 1, 21, 17143, 'Tooltips', DisplayHelpTooltipDelay, 10)
	GUIOPTControls.OptSlider (18018, 2, 22, 17144, 'Mouse Scroll Speed', DisplayHelpMouseScrollingSpeed, 5)
	GUIOPTControls.OptSlider (18019, 3, 23, 17145, 'Keyboard Scroll Speed', None, 5)

	GUIOPTControls.OptSlider (18020, 12, 24, 13911, 'Difficulty Level', None)
	if GemRB.GetVar ("Nightmare Mode") == 1:
		# lock the slider
		Slider = Window.GetControl (12)
		Slider.SetDisabled (True)

	GUIOPTControls.OptCheckbox (18021, 14, 25, 13697, 'Always Dither')
	GUIOPTControls.OptCheckbox (18023, 19, 27, 17182, 'Gore')
	GUIOPTControls.OptCheckbox (11797, 42, 44, 11795, 'Infravision')
	GUIOPTControls.OptCheckbox (20619, 47, 46, 20618, 'Weather')
	if GameCheck.IsBG2():
		GUIOPTControls.OptCheckbox (2242, 50, 48, 2241, 'Heal Party on Rest')
	elif GameCheck.IsIWD2() or GameCheck.IsIWD1():
		GUIOPTControls.OptCheckbox (15136, 50, 49, 17378, 'Maximum HP')

	GUIOPTControls.OptButton (OpenFeedbackOptionsWindow, 5, 17163)
	GUIOPTControls.OptButton (OpenAutopauseOptionsWindow, 6, 17166)
	if GameCheck.IsBG2():
		GUIOPTControls.OptButton (OpenHotkeyOptionsWindow, 51, 816)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DisplayHelpTooltipDelay ():
	GemRB.GetView ("OPTHELP").SetText (18017)
	GemRB.SetTooltipDelay (GemRB.GetVar ("Tooltips") * TOOLTIP_DELAY_FACTOR//10)

def DisplayHelpMouseScrollingSpeed ():
	GemRB.GetView ("OPTHELP").SetText (18018)
	GemRB.SetMouseScrollSpeed (GemRB.GetVar ("Mouse Scroll Speed") )

###################################################

def OpenFeedbackOptionsWindow ():
	"""Open feedback options window"""

	Window = GemRB.LoadWindow (9, "GUIOPT")
	Window.AddAlias("SUB_WIN", 1)
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	GUIOPTControls.OptHelpText (28, 18043)

	GemRB.SetVar ("Circle Feedback", GemRB.GetVar ("GUI Feedback Level") - 1)

	GUIOPTControls.OptDone (Window.Close, 26)
	GUIOPTControls.OptCancel (Window.Close, 27)

	GUIOPTControls.OptSlider (18024, 8, 30, 13688, 'Circle Feedback', DisplayHelpMarkerFeedback)
	GUIOPTControls.OptSlider (18025, 9, 31, 17769, 'Locator Feedback Level')

	# to hit rolls (extra feedback), combat info, actions, state changes, selection text, miscellaneus
	GUIOPTControls.OptCheckbox (18026, 10, 32, 17149, 'Effect Text Level', None, 1)
	GUIOPTControls.OptCheckbox (18027, 11, 33, 17150, 'Effect Text Level', None, 2)
	GUIOPTControls.OptCheckbox (18028, 12, 34, 17151, 'Effect Text Level', None, 4)
	GUIOPTControls.OptCheckbox (18029, 13, 35, 17152, 'Effect Text Level', None, 8)
	GUIOPTControls.OptCheckbox (18030, 14, 36, 17181, 'Effect Text Level', None, 16)
	GUIOPTControls.OptCheckbox (18031, 15, 37, 17153, 'Effect Text Level', None, 32)
	# pst adds bit 64, but it still has its own GUIOPT; let's just ensure it is set
	GemRB.SetVar ('Effect Text Level', GemRB.GetVar ('Effect Text Level') | 64)

	Window.ShowModal (MODAL_SHADOW_GRAY)

	return

def DisplayHelpMarkerFeedback ():
	GemRB.GetView ("OPTHELP").SetText (18024)
	GemRB.SetVar ("GUI Feedback Level", GemRB.GetVar ("Circle Feedback") + 1)

###################################################

def OpenAutopauseOptionsWindow ():
	"""Open autopause options window"""

	Window = GemRB.LoadWindow (10, "GUIOPT")
	Window.AddAlias("SUB_WIN", 1)
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	GUIOPTControls.OptHelpText (15, 18044)

	GUIOPTControls.OptDone (Window.Close, 11)
	GUIOPTControls.OptCancel (Window.Close, 14)

	# checkboxes OR the values if they associate to the same variable
	GUIOPTControls.OptCheckbox (18032, 1, 17, 17155, 'Auto Pause State', None, 4) # hit
	GUIOPTControls.OptCheckbox (18033, 2, 18, 17156, 'Auto Pause State', None, 8) # wounded
	GUIOPTControls.OptCheckbox (18034, 3, 19, 17157, 'Auto Pause State', None, 16) # dead
	GUIOPTControls.OptCheckbox (18035, 4, 20, 17158, 'Auto Pause State', None, 2) # attacked
	GUIOPTControls.OptCheckbox (18036, 5, 21, 17159, 'Auto Pause State', None, 1) # weapon unusable
	GUIOPTControls.OptCheckbox (18037, 13, 22, 17160, 'Auto Pause State', None, 32) # target gone
	GUIOPTControls.OptCheckbox (10640, 25, 24, 10639, 'Auto Pause State', None, 64) # end of round
	if GameCheck.IsIWD2() or GameCheck.IsIWD1():
		GUIOPTControls.OptCheckbox (23514, 30, 31, 23516, 'Auto Pause State', None, 128) # enemy sighted
		GUIOPTControls.OptCheckbox (18560, 26, 28, 16519, 'Auto Pause State', None, 256) # trap found
		GUIOPTControls.OptCheckbox (26311, 36, 37, 26310, 'Auto Pause State', None, 512) # spell cast
		GUIOPTControls.OptCheckbox (24888, 33, 34, 10574, 'Auto Pause Center', None, 1)
	elif Window.GetControl (26):
		GUIOPTControls.OptCheckbox (23514, 26, 27, 23516, 'Auto Pause State', None, 128) # enemy sighted
	if GameCheck.IsBG2():
		GUIOPTControls.OptCheckbox (31872, 31, 33, 31875, 'Auto Pause State', None, 512) # spell cast
		GUIOPTControls.OptCheckbox (58171, 34, 30, 57354, 'Auto Pause State', None, 256) # trap found
		GUIOPTControls.OptCheckbox (10571, 37, 36, 10574, 'Auto Pause Center', None, 1)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def MoviePlayPress():
	movie = MoviesTable.GetRowName (GemRB.GetVar ("MovieIndex"))
	GemRB.PlayMovie (movie, 1)

# for iwd2 only, the rest use GUIMOVIE
# TODO: investigate a merger, so it can get removed from GUIOPT
def OpenMovieWindow ():
	global TextAreaControl, MoviesTable

	Window = GemRB.LoadWindow(2, "GUIMOVIE")
	Window.AddAlias("SUB_WIN", 0)
	Window.SetFlags (WF_BORDERLESS, OP_OR)
	TextAreaControl = Window.GetControl(0)
	PlayButton = Window.GetControl(2)
	CreditsButton = Window.GetControl(3)
	DoneButton = Window.GetControl(4)
	MoviesTable = GemRB.LoadTable("MOVIDESC")
	opts = [MoviesTable.GetValue (i, 0) for i in range(MoviesTable.GetRowCount ()) if GemRB.GetVar(MoviesTable.GetRowName (i)) == 1]
	TextAreaControl.SetColor (ColorWhitish, TA_COLOR_OPTIONS)
	TextAreaControl.SetOptions (opts, "MovieIndex", 0)
	PlayButton.SetText(17318)
	CreditsButton.SetText(15591)
	DoneButton.SetText(11973)
	PlayButton.OnPress (MoviePlayPress)
	CreditsButton.OnPress (lambda: GemRB.PlayMovie("CREDITS"))
	DoneButton.OnPress (Window.Close)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

###################################################

def OpenSaveMsgWindow ():
	GemRB.SetVar("QuitAfterSave",0)
	GUISAVE.OpenSaveWindow ()
	#save the game without quitting
	return

###################################################

def LoadGamePress ():
	GemRB.SetNextScript ("GUILOAD")
	return

#save game AND quit
def SaveGamePress ():
	CloseQuitMsgWindow()
	#we need to set a state: quit after save
	GemRB.SetVar("QuitAfterSave",1)
	GUISAVE.OpenSaveWindow ()
	return

def QuitGamePress ():
	if GemRB.GetVar("AskAndExit") == 1:
		GemRB.Quit()
		return

	CloseQuitMsgWindow()

	GUICommonWindows.CloseTopWindow ()
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

###################################################

def OpenQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		return

	QuitMsgWindow = Window = GemRB.LoadWindow (5, "GUIOPT")
	Window.SetFlags (WF_BORDERLESS, OP_OR)

	# Save
	Button = Window.GetControl (0)
	Button.SetText (15589)
	Button.OnPress (SaveGamePress)
	if GemRB.GetView("GC") is not None:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	# Quit Game
	Button = Window.GetControl (1)
	Button.SetText (15417)
	Button.OnPress (QuitGamePress)
	Button.MakeDefault()

	# Cancel
	Button = Window.GetControl (2)
	Button.SetText (GUIOPTControls.STR_OPT_CANCEL)
	Button.OnPress (CloseQuitMsgWindow)
	Button.MakeEscape()

	# Do you wish to save the game ....
	Text = Window.GetControl (3)
	Text.SetText (16456)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseQuitMsgWindow ():
	global QuitMsgWindow

	if QuitMsgWindow:
		QuitMsgWindow.Close ()
	QuitMsgWindow = None

	GemRB.SetVar("AskAndExit", 0)
	return

###################################################
def OpenHotkeyOptionsWindow ():
	print("TODO: implement OpenHotkeyOptionsWindow!")
	# check if pst's guiopt's OpenKeyboardMappingsWindow is reusable
	return

def CloseHotkeyOptionsWindow ():
	return

###################################################
# End of file GUIOPT.py

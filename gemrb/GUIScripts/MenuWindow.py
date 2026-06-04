# SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# functions for menu/options sidebar window
###################################################

import GemRB
import Clock
import GameCheck
import GUICommon
import GUICommonWindows
from GUIDefines import *

if GameCheck.IsIWD2():
	MageSpellsKey = 'Spellbook'
	CharacterStatsKey = 'Character_Record'
elif GameCheck.IsPST():
	MageSpellsKey = 'Mage_Spells'
	CharacterStatsKey = 'Character_Stats'
else:
	MageSpellsKey = 'Wizard_Spells'
	CharacterStatsKey = 'Character_Record'

# The following tables deal with the different control indexes and string refs of each game
# so that actual interface code can be game neutral
# the dictionary keys match entries in keymap.2da
AITip = {"Deactivate" : 15918, "Enable" : 15917}
if GameCheck.IsPST(): #Torment
	import GUIClasses
	TimeWindow = None
	PortWindow = None
	MenuWindow = None
	MainWindow = None
	DiscWindow = None
	AITip = {	"Deactivate" : 41631, "Enable" : 41646 }
	OptionTip = { # dictionary to the stringrefs in each games dialog.tlk
	'Inventory' : 41601, 'Map': 41625, MageSpellsKey : 41624, 'Priest_Spells': 4709, CharacterStatsKey : 4707, 'Journal': 41623,
	'Options' : 41626, 'Rest': 41628, 'Follow': 41647, 'Expand': 41660, 'Toggle_AI' : 1, 'Return_To_Game' : 1, 'Party' : 1
	}
	OptionControl = { # dictionary to the control indexes in the window (.CHU)
	'Inventory' : 1, 'Map' : 2, MageSpellsKey : 3, 'Priest_Spells': 7, CharacterStatsKey : 5, 'Journal': 6,
	'Options' : 8, 'Rest': 9, 'Follow': 0, 'Expand': 10, 'Toggle_AI': 4,
	'Return_To_Game': 0, 'Party' : 8 , 'Time': 9 # not in pst
	}
elif GameCheck.IsIWD2(): #Icewind Dale 2
	OptionTip = {
	'Inventory' : 16307, 'Map': 16310, MageSpellsKey : 16309, CharacterStatsKey : 16306, 'Journal': 16308,
	'Options' : 16311, 'Rest': 11942, 'Follow': 41647, 'Expand': 41660, 'Toggle_AI' : 1, 'Return_To_Game' : 16313,  'Party' : 16312,
	'SelectAll': 10485
	}
	OptionControl = {
	'Inventory' : 5, 'Map' : 7, CharacterStatsKey : 8, 'Journal': 6,
	'Options' : 9, 'Rest': 12, 'Follow': 0, 'Expand': 10, 'Toggle_AI': 14,
	'Return_To_Game': 0, 'Party' : 13,  'Time': 10, # not in pst
	MageSpellsKey: 4, 'SelectAll': 11
	}
else: # Baldurs Gate, Icewind Dale
	OptionTip = {
	'Inventory' : 16307, 'Map': 16310, MageSpellsKey : 16309, 'Priest_Spells': 14930, CharacterStatsKey : 16306, 'Journal': 16308,
	'Options' : 16311, 'Rest': 11942, 'Follow': 41647,  'Expand': 41660, 'Toggle_AI' : 1, 'Return_To_Game' : 16313, 'Party' : 16312
	}
	OptionControl = {
	'Inventory' : 3, 'Map' : 1, MageSpellsKey : 5, 'Priest_Spells': 6, CharacterStatsKey : 4, 'Journal': 2,
	'Options' : 7, 'Rest': 9, 'Follow': 0, 'Expand': 10, 'Toggle_AI': 6,
	'Return_To_Game': 0, 'Party' : 8, 'Time': 9 # not in pst
	}

# Generic option button init. Pass it the options window. Index is a key to the dicts,
# IsPage means whether the game should mark the button selected
def InitOptionButton(Window, Index, HotKey = True):
	Button = Window.GetControl (OptionControl[Index])
	if not Button:
		print("InitOptionButton cannot find the button: " + Index)
		return

	Button.SetTooltip (OptionTip[Index])
	if HotKey:
		Button.SetHotKey (Index, True)
		
	# this variable isnt used anywhere (tho it might be useful to use it for knowing what window is open)
	# however, we still need to set one because we do depend on the button having a value
	Button.SetVarAssoc ("OPT_BTN", OptionControl[Index])

	return Button

def SetupMenuWindowControls (Window, Gears = None, CloseWindowCallback = None):
	"""Binds all of the basic controls and windows to the options pane."""

	bg1 = GameCheck.IsBG1 ()
	bg2 = GameCheck.IsBG2OrEE ()
	iwd1 = GameCheck.IsIWD1 ()
	how = GameCheck.HasHOW ()
	iwd2 = GameCheck.IsIWD2 ()
	pst = GameCheck.IsPST ()
	#store these instead of doing 50 calls...
	
	EscButton = Window.CreateButton (99, 0, 0, 0, 0);
	EscButton.OnPress (GUICommonWindows.CloseTopWindow)
	EscButton.MakeEscape ()

	if iwd2: # IWD2 has one spellbook to rule them all
		InitOptionButton (Window, MageSpellsKey)

		# AI
		Button = InitOptionButton (Window, 'Toggle_AI')
		Button.OnPress (AIPress)
		AIPress (0) # this initialises the state and tooltip

		# Select All
		Button = InitOptionButton (Window, 'SelectAll', False)
		Button.OnPress (GUICommon.SelectAllOnPress)
	elif pst: # pst has these three controls here instead of portrait pane
		# (Un)Lock view on character
		Button = InitOptionButton (Window, 'Follow', False) # or 41648 Unlock ...
		Button.OnPress (OnLockViewPress)
		# AI
		Button = InitOptionButton(Window, 'Toggle_AI')
		Button.OnPress (AIPress)
		AIPress(0) #this initialises the state and tooltip

		# Message popup FIXME disable on non game screen...
		Button = InitOptionButton (Window, 'Expand', False) # or 41661 Close ...
	else: ## pst lacks this control here. it is on the clock. iwd2 seems to skip it
		# Return to Game
		Button = InitOptionButton (Window, 'Return_To_Game')
		
		if bg1:
			# enabled BAM isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0, 16, 17, 28, 16)
		elif iwd1:
			# disabled/selected frame isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0, 16, 17, 16, 16)

	# Party management / character arbitration. Distinct form reform party window.
	if not pst:
		Button = Window.GetControl (OptionControl['Party'])
		Button.OnPress (None) #TODO: OpenPartyWindow
		if bg1 or bg2:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			Button.SetTooltip (OptionTip['Party'])

	# Map
	Button = InitOptionButton (Window, 'Map')
	if bg1:
		Button.SetSprites ("GUILSOP", 0, 0, 1, 20, 0)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0, 0, 1, 20, 20)

	# Journal
	Button = InitOptionButton (Window, 'Journal')
	if bg1:
		Button.SetSprites ("GUILSOP", 0, 4, 5, 22, 4)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0, 4, 5, 22, 22)

	# Inventory
	Button = InitOptionButton (Window, 'Inventory')
	if bg1:
		Button.SetSprites ("GUILSOP", 0, 2, 3, 21, 2)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0, 2, 3, 21, 21)

	# Records
	Button = InitOptionButton (Window, CharacterStatsKey)
	if bg1:
		Button.SetSprites ("GUILSOP", 0, 6, 7, 23, 6)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0, 6, 7, 23, 23)

	if not iwd2: # All Other Games Have Fancy Distinct Spell Pages
		# Mage
		Button = InitOptionButton (Window, MageSpellsKey)
		if bg1:
			Button.SetSprites ("GUILSOP", 0, 8, 9, 24, 8)
		elif iwd1:
			Button.SetSprites ("GUILSOP", 0, 8, 9, 24, 24)

		# Priest
		Button = InitOptionButton (Window, 'Priest_Spells')
		if bg1:
			Button.SetSprites ("GUILSOP", 0, 10, 11, 25, 10)
		elif iwd1:
			Button.SetSprites ("GUILSOP", 0, 10, 11, 25, 25)

	# Options
	Button = InitOptionButton (Window, 'Options')
	if bg1:
		Button.SetSprites ("GUILSOP", 0, 12, 13, 26, 12)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0, 12, 13, 26, 26)
	ButtonOptionFrame = Button.GetFrame()

	# pause button
	if Gears:
		if how: # how doesn't have this in the right place
			pos = Window.GetFrame()["h"] - 71
			Window.CreateButton (OptionControl['Time'], 0, pos, 64, 71)
		Clock.CreateClockButton(Window.GetControl (OptionControl['Time']))

		if iwd2:
			rb = OptionControl['Rest']
		else:
			rb = 11
	else:
		rb = OptionControl['Rest']

	# BG1 doesn't have a rest button on the main window, so this creates one
	# from what would be the multiplayer arbitration control
	if bg1:
		Button = Window.GetControl (8)
		Button.SetSprites ("GUIRSBUT", 0, 0, 1, 0, 0)
		Button.SetStatus (IE_GUI_BUTTON_ENABLED)
		Button.SetSize (55, 37)
		Button.SetPos (4, 359)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
		rb = 8

	# Rest
	Button = Window.GetControl (rb)
	if Button:
		Button.SetTooltip (OptionTip['Rest'])
		Button.OnPress (RestPress)
	elif rb == 11 and bg2:
		# The 15 gap came from the guiw8.chu that is the network button
		pos = ButtonOptionFrame["y"] + ButtonOptionFrame["h"] + 15
		Button = Window.CreateButton (rb, ButtonOptionFrame["x"], pos, ButtonOptionFrame["w"], ButtonOptionFrame["h"])
		Button.SetSprites ("GUIRSBUT", 0, 0, 1, 0, 0)
		Button.SetTooltip (OptionTip['Rest'])
		Button.OnPress (RestPress)

	UpdateMenuWindowControls ()
	return

# all this for just two special buttons without shaded disabled frames
def UpdateMenuWindowControls ():
	if not GameCheck.IsPST ():
		return

	Window = GemRB.GetView ("OPTWIN")
	pc = GemRB.GameGetFirstSelectedPC ()

	Button = Window.GetControl (OptionControl[MageSpellsKey])
	if GUICommon.CantUseSpellbookWindow (pc):
		Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)
		Button.SetFlags (IE_GUI_BUTTON_SHADE_BASE, OP_OR)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_SHADE_BASE, OP_NAND)

	Button = Window.GetControl (OptionControl["Priest_Spells"])
	if GUICommon.CantUseSpellbookWindow (pc, True):
		Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)
		Button.SetFlags (IE_GUI_BUTTON_SHADE_BASE, OP_OR)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_SHADE_BASE, OP_NAND)
	return

def OnLockViewPress ():
	OptionsWindow = GemRB.GetView ("OPTWIN")
	Button = OptionsWindow.GetControl (0)
	GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_XOR)
	GemRB.GameControlSetScreenFlags (SF_ALWAYSCENTER, OP_XOR)

	# no way to get the screen flags
	if OnLockViewPress.counter % 2:
		# unlock
		Button.SetTooltip (41648)
		Button.SetState (IE_GUI_BUTTON_SELECTED)#dont ask
	else:
		# lock
		Button.SetTooltip (41647)
		Button.SetState (IE_GUI_BUTTON_NORMAL)
	OnLockViewPress.counter += 1

	return

OnLockViewPress.counter = 1

def AIPress (toggle = 1):
	"""Toggles the party AI or refreshes the button state if toggle = 0"""

	if GameCheck.IsPST () or GameCheck.IsIWD2 ():
		OptionsWindow = GemRB.GetView ("OPTWIN")
		Button = OptionsWindow.GetControl (OptionControl['Toggle_AI'])
	else:
		PortraitWin = GemRB.GetView ("PORTWIN")
		Button = PortraitWin.GetControl (OptionControl['Toggle_AI'])

	if toggle:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_XOR)

	AI = GemRB.GetGUIFlags () & GS_PARTYAI
	if AI:
		GemRB.SetVar ("AI", 0)
		Button.SetTooltip (AITip['Deactivate'])
		Button.SetState (IE_GUI_BUTTON_SELECTED)
	else:
		GemRB.SetVar ("AI", GS_PARTYAI)
		Button.SetTooltip (AITip['Enable'])
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	if GameCheck.IsPST ():
		GemRB.SetGlobal ("partyScriptsActive", "GLOBALS", AI)

	# force redrawing, in case a hotkey triggered this function
	Button.SetVarAssoc ("AI", GS_PARTYAI)
	return

def RestPress ():
	GUICommonWindows.CloseTopWindow ()
	# only rest if the dream scripts haven't already
	# bg2 completely offloaded resting to them - if there's a talk, it has to call Rest(Party) itself
	if not GemRB.RunRestScripts ():
		# ensure the scripts run before the actual rest
		GemRB.SetTimedEvent (RealRestPress, 2)

def RealRestPress ():
	# only bg2 has area-based rest movies
	# outside movies start at 2, 1 is for inns
	# 15 means run all checks to see if resting is possible
	info = GemRB.RestParty (15, 0 if GameCheck.IsBG2OrEE () else 2, 1)
	if info["Error"]:
		if GameCheck.IsPST ():
			# open error window
			Window = GemRB.LoadWindow (25, GUICommon.GetWindowPack (), WINDOW_BOTTOM | WINDOW_HCENTER)
			Label = Window.GetControl (0xfffffff) # -1 in the CHU
			Label.SetText (info["ErrorMsg"])
			Button = Window.GetControl (1)
			Button.SetText (1403)
			Button.OnPress (Window.Close)
			Window.ShowModal (MODAL_SHADOW_GRAY)
		else:
			GemRB.DisplayString (info["ErrorMsg"], ColorRed)

	return

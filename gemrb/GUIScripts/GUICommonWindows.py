# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2004 The GemRB Project
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


# GUICommonWindows.py - functions to open common
# windows in lower part of the screen
###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_restype import RES_2DA
from Clock import UpdateClock, CreateClockButton
import GameCheck
import GUICommon
import CommonTables
import CommonWindow
import Container
import InventoryCommon

if GameCheck.IsIWD2():
	MageSpellsKey = 'Spellbook'
	CharacterStatsKey = 'Character_Record'
elif GameCheck.IsPST():
	MageSpellsKey = 'Mage_Spells'
	CharacterStatsKey = 'Character_Stats'
else:
	MageSpellsKey = 'Wizard_Spells'
	CharacterStatsKey = 'Character_Record'

#The following tables deal with the different control indexes and string refs of each game
#so that actual interface code can be game neutral
#the dictionary keys match entries in keymap.2da
AITip = {"Deactivate" : 15918, "Enable" : 15917}
if GameCheck.IsPST(): #Torment
	import GUIClasses
	TimeWindow = None
	PortWindow = None
	MenuWindow = None
	MainWindow = None
	DiscWindow = None
	AITip = {	"Deactivate" : 41631,	"Enable" : 41646 }
	OptionTip = { #dictionary to the stringrefs in each games dialog.tlk
	'Inventory' : 41601,'Map': 41625, MageSpellsKey : 41624, 'Priest_Spells': 4709, CharacterStatsKey : 4707,'Journal': 41623,
	'Options' : 41626,'Rest': 41628,'Follow': 41647,'Expand': 41660,'Toggle_AI' : 1,'Return_To_Game' : 1,'Party' : 1
	}
	OptionControl = { #dictionary to the control indexes in the window (.CHU)
	'Inventory' : 1, 'Map' : 2, MageSpellsKey : 3, 'Priest_Spells': 7, CharacterStatsKey : 5, 'Journal': 6,
	'Options' : 8, 'Rest': 9, 'Follow': 0, 'Expand': 10, 'Toggle_AI': 4,
	'Return_To_Game': 0, 'Party' : 8 , 'Time': 9 #not in pst
	}
elif GameCheck.IsIWD2(): #Icewind Dale 2
	OptionTip = {
	'Inventory' : 16307, 'Map': 16310, MageSpellsKey : 16309, CharacterStatsKey : 16306, 'Journal': 16308,
	'Options' : 16311, 'Rest': 11942, 'Follow': 41647, 'Expand': 41660, 'Toggle_AI' : 1,'Return_To_Game' : 16313,  'Party' : 16312,
	'SelectAll': 10485
	}
	OptionControl = {
	'Inventory' : 5, 'Map' : 7, CharacterStatsKey : 8, 'Journal': 6,
	'Options' : 9, 'Rest': 12, 'Follow': 0, 'Expand': 10, 'Toggle_AI': 14,
	'Return_To_Game': 0, 'Party' : 13,  'Time': 10, #not in pst
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
	'Return_To_Game': 0, 'Party' : 8, 'Time': 9 #not in pst
	}

# Generic option button init. Pass it the options window. Index is a key to the dicts,
# IsPage means whether the game should mark the button selected
def InitOptionButton(Window, Index, HotKey=True):
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

##these defaults don't seem to break the games other than pst
def SetupMenuWindowControls (Window, Gears=None, CloseWindowCallback=None):
	"""Binds all of the basic controls and windows to the options pane."""

	bg1 = GameCheck.IsBG1()
	bg2 = GameCheck.IsBG2OrEE ()
	iwd1 = GameCheck.IsIWD1()
	how = GameCheck.HasHOW()
	iwd2 = GameCheck.IsIWD2()
	pst = GameCheck.IsPST()
	#store these instead of doing 50 calls...
	
	EscButton = Window.CreateButton (99, 0, 0, 0, 0);
	EscButton.OnPress (CloseTopWindow)
	EscButton.MakeEscape()

	if iwd2: # IWD2 has one spellbook to rule them all
		InitOptionButton(Window, MageSpellsKey)

		# AI
		Button = InitOptionButton(Window, 'Toggle_AI')
		Button.OnPress (AIPress)
		AIPress (0) #this initialises the state and tooltip

		# Select All
		Button = InitOptionButton(Window, 'SelectAll', False)
		Button.OnPress (GUICommon.SelectAllOnPress)
	elif pst: #pst has these three controls here instead of portrait pane
		# (Un)Lock view on character
		Button = InitOptionButton(Window, 'Follow', False)  # or 41648 Unlock ...
		Button.OnPress (OnLockViewPress)
		# AI
		Button = InitOptionButton(Window, 'Toggle_AI')
		Button.OnPress (AIPress)
		AIPress(0) #this initialises the state and tooltip

		# Message popup FIXME disable on non game screen...
		Button = InitOptionButton(Window, 'Expand', False)# or 41661 Close ...

	else: ## pst lacks this control here. it is on the clock. iwd2 seems to skip it
		# Return to Game
		Button = InitOptionButton(Window,'Return_To_Game')
		
		if bg1:
			# enabled BAM isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,28,16)
		elif iwd1:
			# disabled/selected frame isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,16,16)

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
	Button = InitOptionButton(Window, 'Map')
	if bg1:
		Button.SetSprites ("GUILSOP", 0,0,1,20,0)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0,0,1,20,20)

	# Journal
	Button = InitOptionButton(Window, 'Journal')
	if bg1:
		Button.SetSprites ("GUILSOP", 0,4,5,22,4)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0,4,5,22,22)

	# Inventory
	Button = InitOptionButton(Window, 'Inventory')
	if bg1:
		Button.SetSprites ("GUILSOP", 0,2,3,21,2)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0,2,3,21,21)

	# Records
	Button = InitOptionButton(Window, CharacterStatsKey)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,6,7,23,6)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0,6,7,23,23)

	if not iwd2: # All Other Games Have Fancy Distinct Spell Pages
		# Mage
		Button = InitOptionButton(Window, MageSpellsKey)
		pc = GemRB.GameGetSelectedPCSingle ()
		if bg1:
			Button.SetSprites ("GUILSOP", 0,8,9,24,8)
		elif iwd1:
			Button.SetSprites ("GUILSOP", 0,8,9,24,24)
		elif pst:
			# these two blocks do nothing yet
			# the disabled frames are there, but they are identical to pressed
			if GUICommon.CantUseSpellbookWindow (pc):
				Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_ENABLED)

		# Priest
		Button = InitOptionButton(Window, 'Priest_Spells')
		if bg1:
			Button.SetSprites ("GUILSOP", 0,10,11,25,10)
		elif iwd1:
			Button.SetSprites ("GUILSOP", 0,10,11,25,25)
		elif pst:
			if GUICommon.CantUseSpellbookWindow (pc, True):
				Button.SetState (IE_GUI_BUTTON_FAKEDISABLED)
			else:
				Button.SetState (IE_GUI_BUTTON_ENABLED)


	# Options
	Button = InitOptionButton(Window, 'Options')
	if bg1:
		Button.SetSprites ("GUILSOP", 0,12,13,26,12)
	elif iwd1:
		Button.SetSprites ("GUILSOP", 0,12,13,26,26)
	ButtonOptionFrame = Button.GetFrame()

	# pause button
	if Gears:
		if how: # how doesn't have this in the right place
			pos = Window.GetFrame()["h"] - 71
			Window.CreateButton (OptionControl['Time'], 0, pos, 64, 71)
		CreateClockButton(Window.GetControl(OptionControl['Time']))

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
		Button.SetSprites("GUIRSBUT", 0,0,1,0,0)
		Button.SetStatus (IE_GUI_BUTTON_ENABLED)
		Button.SetSize(55,37)
		Button.SetPos(4,359)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_SET)
		rb = 8

	# Rest
	Button = Window.GetControl (rb)
	if Button:
		Button.SetTooltip (OptionTip['Rest'])
		Button.OnPress (RestPress)
	elif rb == 11 and bg2:
		#The 15 gap came from the guiw8.chu that is the network button
		pos = ButtonOptionFrame["y"] + ButtonOptionFrame["h"] + 15
		Button = Window.CreateButton (rb, ButtonOptionFrame["x"], pos, ButtonOptionFrame["w"], ButtonOptionFrame["h"])
		Button.SetSprites ("GUIRSBUT", 0,0,1,0,0)
		Button.SetTooltip (OptionTip['Rest'])
		Button.OnPress (RestPress)
	return

def OnLockViewPress ():
	OptionsWindow = GemRB.GetView("OPTWIN")
	Button = OptionsWindow.GetControl (0)
	GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_XOR)
	GemRB.GameControlSetScreenFlags (SF_ALWAYSCENTER, OP_XOR)

	# no way to get the screen flags
	if OnLockViewPress.counter % 2:
		# unlock
		Button.SetTooltip (41648)
		Button.SetState(IE_GUI_BUTTON_SELECTED)#dont ask
	else:
		# lock
		Button.SetTooltip (41647)
		Button.SetState(IE_GUI_BUTTON_NORMAL)
	OnLockViewPress.counter += 1

	return

OnLockViewPress.counter = 1

def PortraitPress ():
	"""Toggles the portraits pane """
	PP = GemRB.GetGUIFlags () & GS_PORTRAITPANE
	if PP:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_NAND)
	else:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_OR)
	return

def AIPress (toggle=1):
	"""Toggles the party AI or refreshes the button state if toggle = 0"""

	if GameCheck.IsPST() or GameCheck.IsIWD2():
		OptionsWindow = GemRB.GetView("OPTWIN")
		Button = OptionsWindow.GetControl (OptionControl['Toggle_AI'])
	else:
		PortraitWin = GemRB.GetView("PORTWIN")
		Button = PortraitWin.GetControl (OptionControl['Toggle_AI'])

	if toggle:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_XOR)

	AI = GemRB.GetGUIFlags () & GS_PARTYAI
	if AI:
		GemRB.SetVar ("AI", 0)
		Button.SetTooltip (AITip['Deactivate'])
		Button.SetState(IE_GUI_BUTTON_SELECTED)
	else:
		GemRB.SetVar ("AI", GS_PARTYAI)
		Button.SetTooltip (AITip['Enable'])
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	if GameCheck.IsPST ():
		GemRB.SetGlobal ("partyScriptsActive", "GLOBALS", AI)

	#force redrawing, in case a hotkey triggered this function
	Button.SetVarAssoc ("AI", GS_PARTYAI)
	return

# NOTE: the following two features are only used in pst
# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	#return GemRB.GetPlayerPortrait( actor, which)

	# only the lowest byte is meaningful here (OneByteAnimID)
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID) & 255
	row = "0x%02X" %anim_id

	return CommonTables.Pdolls.GetValue (row, which)


def UpdateAnimation ():
	if not GemRB.HasResource ("ANIMS", RES_2DA, 1):
		# FIXME: make a simpler version for non-pst too
		# this is a callback from the core on EF_UPDATEANIM!
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	disguise = GemRB.GetGameVar ("APPEARANCE")
	if disguise == 2: #dustman
		animid = "DR"
	elif disguise == 1: #zombie
		animid = "ZO"
	else:
		slot = GemRB.GetEquippedQuickSlot (pc)
		item = GemRB.GetSlotItem (pc, slot )
		animid = ""
		if item:
			item = GemRB.GetItem(item["ItemResRef"])
			if item:
				animid = item["AnimationType"]

	BioTable = GemRB.LoadTable ("BIOS")
	Specific = GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = BioTable.GetRowName (Specific)
	AnimTable = GemRB.LoadTable ("ANIMS")
	if animid=="":
		animid="*"
	value = AnimTable.GetValue (animid, AvatarName, GTV_INT)
	if value<0:
		return
	GemRB.SetPlayerStat (pc, IE_ANIMATION_ID, value)
	return

# NOTE: the following 4 functions are only used in iwd2
def GetActorRaceTitle (actor):
	import IDLUCommon
	RaceIndex = IDLUCommon.GetRace (actor)
	RaceTitle = CommonTables.Races.GetValue (RaceIndex, 2)
	return RaceTitle

# NOTE: this function is called with the primary classes
def GetKitIndex (actor, ClassIndex):
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)

	KitIndex = -1
	ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	ClassID = CommonTables.Classes.GetValue (ClassName, "ID")
	# skip the primary classes
	# start at the first original kit - in iwd2 both classes and kits are in the same table
	KitOffset = CommonTables.Classes.FindValue ("CLASS", 7)
	for ci in range (KitOffset, CommonTables.Classes.GetRowCount ()):
		RowName = CommonTables.Classes.GetRowName (ci)
		BaseClass = CommonTables.Classes.GetValue (RowName, "CLASS")
		if BaseClass == ClassID and Kit & CommonTables.Classes.GetValue (RowName, "ID", GTV_INT):
			#FIXME: this will return the last kit only, check if proper multikit return values are needed
			KitIndex = ci

	if KitIndex == -1:
		return 0

	return KitIndex

def GetActorClassTitle (actor, ClassIndex):
	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)
	if ClassTitle:
		return ClassTitle

	KitIndex = GetKitIndex (actor, ClassIndex)
	if KitIndex == 0:
		ClassName = CommonTables.Classes.GetRowName (ClassIndex)
	else:
		ClassName = CommonTables.Classes.GetRowName (KitIndex)
	ClassTitle = CommonTables.Classes.GetValue (ClassName, "NAME_REF")

	if ClassTitle == "*":
		return 0
	return ClassTitle

# overriding the one in GUICommon, since we use a different table and animations
def GetActorPaperDoll (actor):
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	return GemRB.GetAvatarsValue (actor, level)


SelectionChangeHandler = None

def SetSelectionChangeHandler (handler):
	"""Updates the selection handler."""

	global SelectionChangeHandler

	# Switching from walking to non-walking environment:
	# set the first selected PC in walking env as a selected
	# in nonwalking env
	#if (not SelectionChangeHandler) and handler:
	if (not SelectionChangeHandler) and handler:
		sel = GemRB.GameGetFirstSelectedPC ()
		if sel:
			GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	return

def CloseTopWindow ():
	window = GemRB.GetView("WIN_TOP")
	# we cannot close the current WIN_TOP unless it has focus
	if window and window.HasFocus == True:
		window.Close()
		UpdateClock()

def TopWindowClosed(window):
	optwin = GemRB.GetView("OPTWIN")
	btnid = GemRB.GetVar("OPTBTN")
	button = optwin.GetControl(btnid) if optwin else None
	if button:
		button.SetState(IE_GUI_BUTTON_UNPRESSED)
	rtgbtn = optwin.GetControl(0) if optwin else None # return to game button
	if rtgbtn: # not in PST or IWD2
		rtgbtn.SetState(IE_GUI_BUTTON_SELECTED)
		
	print("pause state " + str(CreateTopWinLoader.PauseState))
	if CreateTopWinLoader.PauseState is not None:
		GemRB.GamePause (CreateTopWinLoader.PauseState, 3)

	GameWin = GemRB.GetView("GAMEWIN")
	GameWin.SetDisabled(False)

	if not CommonWindow.IsGameGUIHidden() and GemRB.GetView ("ACTWIN") and not (GameCheck.IsIWD2() or GameCheck.IsPST()):
		GemRB.GetView ("ACTWIN").SetVisible (True)
		GemRB.GetView ("MSGWIN").SetVisible(True)

	SetSelectionChangeHandler (None)
	SelectionChanged()

if GameCheck.IsIWD2():
	DefaultWinPos = WINDOW_TOP|WINDOW_HCENTER
elif GameCheck.IsPST () and GemRB.GetSystemVariable (SV_HEIGHT) < 480 + 73:
	DefaultWinPos = WINDOW_TOP | WINDOW_HCENTER
else:
	DefaultWinPos = WINDOW_CENTER

def CreateTopWinLoader(id, pack, loader, initer = None, selectionHandler = None, pause = False, pos = DefaultWinPos):
	def ret (btn = None):
		topwin = GemRB.GetView("WIN_TOP")
		if topwin and topwin.HasFocus == False:
			return None # we cannot close the current WIN_TOP unless it has focus
	
		window = loader(id, pack, pos)

		if window:
			# set this before calling initer so initer can change it if it wants
			window.SetFlags(WF_ALPHA_CHANNEL, OP_NAND)
			if initer:
				initer(window)
	
			if selectionHandler:
				selectionHandler(window)
				window.OnFocus (selectionHandler)
			
			SetTopWindow (window, selectionHandler)
			window.OnClose (lambda: TopWindowClosed(window))

			if pause:
				CreateTopWinLoader.PauseState = GemRB.GamePause(3, 1)
				GemRB.GamePause (1, 3)
			else:
				CreateTopWinLoader.PauseState = None

			optwin = GemRB.GetView("OPTWIN")
			if optwin:
				if btn:
					btn.SetState(IE_GUI_BUTTON_SELECTED)
					GemRB.SetVar ("OPTBTN", btn.ID)
				rtgbtn = optwin.GetControl(0) # return to game button
				if rtgbtn: # not in PST or IWD2
					rtgbtn.SetState(IE_GUI_BUTTON_UNPRESSED)
			
			GameWin = GemRB.GetView("GAMEWIN")
			GameWin.SetDisabled(True)
			
			if not CommonWindow.IsGameGUIHidden() and GemRB.GetView ("ACTWIN") and not (GameCheck.IsIWD2() or GameCheck.IsPST()):
				# hide some windows to declutter higher resolutions and avoid unwanted interaction
				GemRB.GetView ("ACTWIN").SetVisible(False)
				GemRB.GetView ("MSGWIN").SetVisible(False)

		return window
	
	return ret

def SetTopWindow (window, selectionHandler = None):
	topwin = GemRB.GetView("WIN_TOP")
	if topwin == window:
		return
		
	pc = GemRB.GameGetSelectedPCSingle()
	
	if topwin:
		topwin.Close() # invalidates topwin so must use a different variable
		preserveSelection = True
	else:
		preserveSelection = False

	if window:
		window.AddAlias("WIN_TOP")
		window.Focus()

		UpdateClock()

		if selectionHandler:
			selectionHandler = lambda win=window, fn=selectionHandler: fn(win)

		SetSelectionChangeHandler (selectionHandler)
		if preserveSelection:
			# we are going to another topwin so preserve the selection
			GemRB.GameSelectPCSingle(pc)
	else:
		SetSelectionChangeHandler (None)

def OpenWindowOnce(id, pack, pos=WINDOW_CENTER):
	window = GemRB.GetView(pack, id)
	if window:
		window.Focus()
		return None
	else:
		return GemRB.LoadWindow(id, pack, pos)

def ToggleWindow(id, pack, pos=WINDOW_CENTER):
	# do nothing while in a store
	if GemRB.GetView ("WIN_STORE"):
		return None

	window = GemRB.GetView(pack, id)
	if window:
		window.Close()
		UpdateClock ()
		return None
	else:
		return GemRB.LoadWindow(id, pack, pos)

def OpenInventoryWindowClick (btn):
	import GUIINV
	GemRB.GameSelectPC (btn.Value, True, SELECT_REPLACE)
	GUIINV.OpenInventoryWindow ()
	return

DragButton = None
def ButtonDragSourceHandler (btn):
	global DragButton
	DragButton = btn;
	
def ButtonDragDestHandler (btn):
	global DragButton
	
	pcID = btn.Value
	# So far this is only used for portrait buttons
	if DragButton: # DragButton will be none for item drags instantiated by clicking (not dragging) an inventory item
		if DragButton.VarName == "portrait" and btn.VarName == "portrait":
			GemRB.GameSwapPCs (DragButton.Value, pcID)
			
	else: # TODO: something like this: DragButton.VarName.startswith("slot")
		if btn.VarName == "portrait":
			InventoryCommon.OnDropItemToPC(pcID)
		# TODO: handle inventory/ground slots
		
	DragButton = None

def PortraitButtonOnPress (btn):
	"""Selects the portrait individually."""
	pcID = btn.Value

	if GemRB.GameControlGetTargetMode() != TARGET_MODE_NONE:
		GemRB.ActOnPC (pcID)
		return

	if (not SelectionChangeHandler):
		if GemRB.GameIsPCSelected (pcID):
			GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR)
		GemRB.GameSelectPC (pcID, True, SELECT_REPLACE)
	else:
		GemRB.GameSelectPCSingle (pcID)
	return

def PortraitButtonOnShiftPress (btn):
	"""Handles selecting multiple portraits with shift."""
	
	pcID = btn.Value
	if (not SelectionChangeHandler):
		sel = GemRB.GameIsPCSelected (pcID)
		sel = not sel
		GemRB.GameSelectPC (pcID, sel)
	else:
		GemRB.GameSelectPCSingle (pcID)
	return

def SelectionChanged ():
	"""Ran by the Game class when a PC selection is changed."""
	from PortraitWindow import UpdatePortraitWindow
	import ActionsWindow

	# FIXME: hack. If defined, display single selection
	ActionsWindow.SetActionLevel (UAW_STANDARD)
	if (not SelectionChangeHandler):
		import ActionsWindow
		ActionsWindow.UpdateActionsWindow ()
	else:
		pc = GemRB.GameGetSelectedPCSingle();
		GUICommon.UpdateMageSchool (pc)

	UpdatePortraitWindow()
	Container.CloseContainerWindow()
	if SelectionChangeHandler:
		SelectionChangeHandler ()
	return

def OpenWaitForDiscWindow (disc_num):
	global DiscWindow

	if DiscWindow:
		DiscWindow.Close ()
		DiscWindow = None
		return

	DiscWindow = GemRB.LoadWindow (0, "GUIID")
	label = DiscWindow.GetControl (0)

	disc_path = 'XX:'

	text = GemRB.GetString (31483) + " " + str (disc_num) + " " + GemRB.GetString (31569) + " " + disc_path + "\n" + GemRB.GetString (49152)
	label.SetText (text)
	# 31483 - Please place PS:T disc number
	# 31568 - Please place the PS:T DVD
	# 31569 - in drive
	# 31570 - Wrong disc in drive
	# 31571 - There is no disc in drive
	# 31578 - No disc could be found in drive. Please place Disc 1 in drive.
	# 49152 - To quit the game, press Alt-F4

def ToggleAlwaysRun():
	GemRB.GameControlToggleAlwaysRun()

def RestPress ():
	CloseTopWindow ()
	# only rest if the dream scripts haven't already
	# bg2 completely offloaded resting to them - if there's a talk, it has to call Rest(Party) itself
	if not GemRB.RunRestScripts ():
		# ensure the scripts run before the actual rest
		GemRB.SetTimedEvent (RealRestPress, 2)

def RealRestPress ():
	# only bg2 has area-based rest movies
	# outside movies start at 2, 1 is for inns
	# 15 means run all checks to see if resting is possible
	info = GemRB.RestParty(15, 0 if GameCheck.IsBG2OrEE () else 2, 1)
	if info["Error"]:
		if GameCheck.IsPST ():
			# open error window
			Window = GemRB.LoadWindow (25, GUICommon.GetWindowPack (), WINDOW_BOTTOM|WINDOW_HCENTER)
			Label = Window.GetControl (0xfffffff) # -1 in the CHU
			Label.SetText (info["ErrorMsg"])
			Button = Window.GetControl (1)
			Button.SetText (1403)
			Button.OnPress (Window.Close)
			Window.ShowModal (MODAL_SHADOW_GRAY)
		else:
			GemRB.DisplayString (info["ErrorMsg"], ColorRed)

	return

# special pst death screen for the finale
def OpenPSTDeathWindow ():
	if not GameCheck.IsPST ():
		return

	def ShowCredits():
		GemRB.ExecuteString ("EndCredits()")
		# will also exit to the main menu

	# reuse the main error window
	Window = GemRB.LoadWindow (25, GUICommon.GetWindowPack())
	Label = Window.GetControl (0xfffffff) # -1 in the CHU
	Label.SetText (48155)
	Button = Window.GetControl (1)
	Button.SetText (1403)
	Button.OnPress (ShowCredits)
	Window.ShowModal (MODAL_SHADOW_GRAY)

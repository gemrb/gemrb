# -*-python-*-
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
from ie_modal import *
from ie_action import *
from ie_slots import SLOT_QUIVER
import GUICommon
import CommonTables
import LUCommon
import InventoryCommon
if not GUICommon.GameIsPST():
  import Spellbook  ##not used in pst - YET

# needed for all the Open*Window callbacks in the OptionsWindow
import GUIJRNL
import GUIMA
import GUIINV
import GUIOPT
if GUICommon.GameIsIWD2():
	# one spellbook for all spell types
	import GUISPL
else:
	import GUIMG
	import GUIPR
import GUIREC

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

PortraitWindow = None
OptionsWindow = None
ActionsWindow = None
CurrentWindow = None
DraggedPortrait = None
ActionBarControlOffset = 0

#The following tables deal with the different control indexes and string refs of each game
#so that actual interface code can be game neutral
if GUICommon.GameIsPST(): #Torment
	import GUIClasses
	TimeWindow = None
	PortWindow = None
	MenuWindow = None
	MainWindow = None
	DiscWindow = None
	AITip = {	"Deactivate" : 41631,	"Enable" : 41646 }
	OptionTip = { #dictionary to the stringrefs in each games dialog.tlk
	'Inventory' : 41601,'Map': 41625,'Mage': 41624,'Priest': 4709,'Stats': 4707,'Journal': 41623,
	'Options' : 41626,'Rest': 41628,'Follow': 41647,'Expand': 41660,'AI' : 1,'Game' : 1,'Party' : 1
	}
	OptionControl = { #dictionary to the control indexes in the window (.CHU)
	'Inventory' : 1, 'Map' : 2, 'Mage': 3, 'Priest': 7, 'Stats': 5, 'Journal': 6,
	'Options' : 8, 'Rest': 9, 'Follow': 0, 'Expand': 10, 'AI': 4,
	'Game': 0, 'Party' : 8 , 'Time': 9 #not in pst
	}
elif GUICommon.GameIsIWD2(): #Icewind Dale 2
	OptionTip = {
	'Inventory' : 16307, 'Map': 16310, 'Mage': 16309, 'Priest': 14930, 'Stats': 16306, 'Journal': 16308,
	'Options' : 16311, 'Rest': 11942, 'Follow': 41647, 'Expand': 41660, 'AI' : 1,'Game' : 16313,  'Party' : 16312,
	'SpellBook': 16309, 'SelectAll': 10485
	}
	OptionControl = {
	'Inventory' : 5, 'Map' : 7, 'Mage': 5, 'Priest': 6, 'Stats': 8, 'Journal': 6,
	'Options' : 9, 'Rest': 12, 'Follow': 0, 'Expand': 10, 'AI': 6,
	'Game': 0, 'Party' : 13,  'Time': 10, #not in pst
	'SpellBook': 4, 'SelectAll': 11
	}
else: # Baldurs Gate, Icewind Dale
	AITip = {"Deactivate" : 15918, "Enable" : 15917}
	OptionTip = {
	'Inventory' : 16307, 'Map': 16310, 'Mage': 16309, 'Priest': 14930, 'Stats': 16306, 'Journal': 16308,
	'Options' : 16311, 'Rest': 11942, 'Follow': 41647,  'Expand': 41660, 'AI' : 1, 'Game' : 16313, 'Party' : 16312
	}
	OptionControl = {
	'Inventory' : 3, 'Map' : 1, 'Mage': 5, 'Priest': 6, 'Stats': 4, 'Journal': 2, 
	'Options' : 7, 'Rest': 9, 'Follow': 0, 'Expand': 10, 'AI': 6,
	'Game': 0, 'Party' : 8, 'Time': 9 #not in pst
	}

# Generic option button init. Pass it the options window. Index is a key to the dicts,
# IsPage means whether the game should mark the button selected
def InitOptionButton(Window, Index, Action=0,IsPage=1):
	if not Window.HasControl(OptionControl[Index]):
		print "InitOptionButton cannot find the button: " + Index
		return

	Button = Window.GetControl (OptionControl[Index])
	# FIXME: add "(key)" to tooltips!
	Button.SetTooltip (OptionTip[Index])
	if Action:
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, Action)
	if IsPage:
		Button.SetVarAssoc ("SelectedWindow", OptionControl[Index])
	return Button

##these defaults don't seem to break the games other than pst
def SetupMenuWindowControls (Window, Gears=None, ReturnToGame=None):
	"""Binds all of the basic controls and windows to the options pane."""

	global OptionsWindow, ActionBarControlOffset

	OptionsWindow = Window

	bg1 = GUICommon.GameIsBG1()
	bg2 = GUICommon.GameIsBG2()
	iwd1 = GUICommon.GameIsIWD1()
	how = GUICommon.HasHOW()
	iwd2 = GUICommon.GameIsIWD2()
	pst = GUICommon.GameIsPST()
	#store these instead of doing 50 calls...

	if iwd2: # IWD2 has one spellbook to rule them all
		ActionBarControlOffset = 6 #portrait and action window were merged
		# Spellbook
		Button = InitOptionButton(Window, 'SpellBook', GUISPL.OpenSpellBookWindow)
		if Gears: # todo: don't know if it needs this if or if it's if'fing around
			# Select All
			Button = InitOptionButton(Window, 'SelectAll', GUICommon.SelectAllOnPress)

	elif pst: #pst has these three controls here instead of portrait pane
		# (Un)Lock view on character
		Button = InitOptionButton(Window, 'Follow', OnLockViewPress)  # or 41648 Unlock ...
		# AI
		Button = InitOptionButton(Window, 'AI', AIPress)
		AIPress(toggle=0)
		# Message popup FIXME disable on non game screen...
		Button = InitOptionButton(Window,'Expand')# or 41661 Close ...

	else: ## pst lacks this control here. it is on the clock. iwd2 seems to skip it
		# Return to Game
		Button = InitOptionButton(Window,'Game', ReturnToGame)
		Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
		if bg1:
			# enabled BAM isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,28,16)
		if iwd1:
			# disabled/selected frame isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,16,16)

	# Party managment / character arbitration. Distinct form reform party window.
	if not pst:
		Button = Window.GetControl (OptionControl['Party'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: OpenPartyWindow
		if bg1 or bg2:
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			Button.SetTooltip (OptionTip['Party'])

	# Map
	Button = InitOptionButton(Window, 'Map', GUIMA.OpenMapWindow)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,0,1,20,0)
	if iwd1:
		Button.SetSprites ("GUILSOP", 0,0,1,20,20)

	# Journal
	Button = InitOptionButton(Window, 'Journal', GUIJRNL.OpenJournalWindow)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,4,5,22,4)
	if iwd1:
		Button.SetSprites ("GUILSOP", 0,4,5,22,22)

	# Inventory
	Button = InitOptionButton(Window, 'Inventory', GUIINV.OpenInventoryWindow)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,2,3,21,2)
	if iwd1:
		Button.SetSprites ("GUILSOP", 0,2,3,21,21)

	# Records
	Button = InitOptionButton(Window, 'Stats', GUIREC.OpenRecordsWindow)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,6,7,23,6)
	if iwd1:
		Button.SetSprites ("GUILSOP", 0,6,7,23,23)

	if not iwd2: # All Other Games Have Fancy Distinct Spell Pages
		# Mage
		Button = InitOptionButton(Window, 'Mage', GUIMG.OpenMageWindow)
		if bg1:
			Button.SetSprites ("GUILSOP", 0,8,9,24,8)
		if iwd1:
			Button.SetSprites ("GUILSOP", 0,8,9,24,24)

		# Priest
		Button = InitOptionButton(Window, 'Priest', GUIPR.OpenPriestWindow)
		if bg1:
			Button.SetSprites ("GUILSOP", 0,10,11,25,10)
		if iwd1:
			Button.SetSprites ("GUILSOP", 0,10,11,25,25)

	# Options
	Button = InitOptionButton(Window, 'Options', GUIOPT.OpenOptionsWindow)
	if bg1:
		Button.SetSprites ("GUILSOP", 0,12,13,26,12)
	if iwd1:
		Button.SetSprites ("GUILSOP", 0,12,13,26,26)


	# pause button
	if Gears:
		# Pendulum, gears, sun/moon dial (time)
		# FIXME: display all animations: CPEN, CGEAR, CDIAL
		if how: # how doesn't have this in the right place
			pos = GemRB.GetSystemVariable (SV_HEIGHT)-71
			Window.CreateButton (9, 6, pos, 64, 71)
		Button = Window.GetControl (OptionControl['Time'])
		if bg2:
			Label = Button.CreateLabelOnButton (0x10000009, "NORMAL", 0)
			Label.SetAnimation ("CPEN")

		Button.SetAnimation ("CGEAR")
		if bg2:
			Button.SetBAM ("CDIAL", 0, 0)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
		GUICommon.SetGamedaysAndHourToken()
		Button.SetTooltip(GemRB.GetString (16041))
		if iwd2:
			Button.SetState (IE_GUI_BUTTON_LOCKED) #no button depression, timer is an inset stone planet
			rb = OptionControl['Rest']
		else:
			rb = 11
	else:
		rb = OptionControl['Rest']

	# Rest
	if Window.HasControl (rb):
		Button = Window.GetControl (rb)
		Button.SetTooltip (OptionTip['Rest'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

	if bg1 or iwd1 or bg2:
		MarkMenuButton (Window)

	if PortraitWindow:
		UpdatePortraitWindow ()
	return

def MarkMenuButton (WindowIndex):
	Pressed = WindowIndex.GetControl( GemRB.GetVar ("SelectedWindow") )

	for button in range (9):
		Button = WindowIndex.GetControl (button)
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	if Pressed:
		Button = Pressed
	else: # highlight return to game
		Button = WindowIndex.GetControl (0)

	# NOTE: Alternatively, comment out this block or add a feature check, so that
	#   clicking button the second time closes a window again, which might be preferred
	if GUICommon.GameIsIWD1() and GemRB.GetVar ("SelectedWindow") != 0:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		return

	Button.SetState (IE_GUI_BUTTON_SELECTED)

	return

def OptionsPress ():
	"""Toggles the options pane """
	PP = GemRB.GetMessageWindowSize () & GS_OPTIONPANE
	if PP:
		GemRB.GameSetScreenFlags (GS_OPTIONPANE, OP_NAND)
	else:
		GemRB.GameSetScreenFlags (GS_OPTIONPANE, OP_OR)

def OnLockViewPress ():
	Button = OptionsWindow.GetControl (0)
	GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR | SF_ALWAYSCENTER, OP_XOR)

	# no way to get the screen flags
	if OnLockViewPress.counter % 2:
		# unlock
		Button.SetTooltip (41648)
	else:
		# lock
		Button.SetTooltip (41647)
	OnLockViewPress.counter += 1

	return

OnLockViewPress.counter = 1

def AIPress (toggle=1):
	Button = OptionsWindow.GetControl (4)

	if toggle:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_XOR)
	AI = GemRB.GetMessageWindowSize () & GS_PARTYAI

	if AI:
		# disactivate
		Button.SetTooltip (41631)
	else:
		# activate
		Button.SetTooltip (41646)

def TxtePress ():
	print "TxtePress"

def OpenActionsWindowControls (Window): #FIXME:unused in pst. one day could be?
	global ActionsWindow

	ActionsWindow = Window
	# 1280 and higher don't have this control
	if not Window.HasControl (62):
		UpdateActionsWindow ()
		return
	# Gears (time) when options pane is down
	Button = Window.GetControl (62)
	Label = Button.CreateLabelOnButton (0x1000003e, "NORMAL", 0)

	# FIXME: display all animations
	Label.SetAnimation ("CPEN")
	Button.SetAnimation ("CGEAR")
	Button.SetBAM ("CDIAL", 0, 0)
	Button.SetState (IE_GUI_BUTTON_ENABLED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
	GUICommon.SetGamedaysAndHourToken()
	Button.SetTooltip(GemRB.GetString (16041))
	UpdateActionsWindow ()
	return

def SetupClockWindowControls (Window):
	global ActionsWindow

	ActionsWindow = Window
	# time button
	Button = Window.GetControl (0)
	Button.SetAnimation ("WMTIME")
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
	Button.SetEvent(IE_GUI_MOUSE_ENTER_BUTTON, UpdateClock)
	SetPSTGamedaysAndHourToken ()
	Button.SetTooltip (GemRB.GetString(65027))

	# 41627 - Return to the Game World
	Button = Window.GetControl (2)
	Button.SetState (IE_GUI_BUTTON_DISABLED)
	Button.SetTooltip (41627)

	# Select all characters
	Button = Window.GetControl (1)
	Button.SetTooltip (41659)

	# Abort current action
	Button = Window.GetControl (3)
	Button.SetTooltip (41655)

	# Formations
	Button = Window.GetControl (4)
	Button.SetTooltip (44945)

	return


# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	#return GemRB.GetPlayerPortrait( actor, which)

	# only the lowest byte is meaningful here (OneByteAnimID)
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID) & 255
	row = "0x%02X" %anim_id

	return CommonTables.Pdolls.GetValue (row, which)


def UpdateAnimation ():
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
	Specific = "%d"%GemRB.GetPlayerStat (pc, IE_SPECIFIC)
	AvatarName = BioTable.GetValue (Specific, "PC")
	AnimTable = GemRB.LoadTable ("ANIMS")
	if animid=="":
		animid="*"
	value = AnimTable.GetValue (animid, AvatarName)
	if value<0:
		return
	GemRB.SetPlayerStat (pc, IE_ANIMATION_ID, value)
	return


SelectionChangeHandler = None
SelectionChangeMultiHandler = None

def SetSelectionChangeHandler (handler):
	"""Updates the selection handler."""

	global SelectionChangeHandler

	# Switching from walking to non-walking environment:
	# set the first selected PC in walking env as a selected
	# in nonwalking env
	#if (not SelectionChangeHandler) and handler and (not GUICommon.NextWindowFn):
	if (not SelectionChangeHandler) and handler:
		sel = GemRB.GameGetFirstSelectedPC ()
		if not sel:
			sel = 1
		GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	# redraw selection on change main selection | single selection
	SelectionChanged ()

def SetSelectionChangeMultiHandler (handler):
	global SelectionChangeMultiHandler
	SelectionChangeMultiHandler = handler
	#SelectionChanged ()

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()

portrait_hp_numeric = [0, 0, 0, 0, 0, 0]


def OpenPortraitWindow (needcontrols):
	global PortraitWindow

	PortraitWindow = Window = GemRB.LoadWindow (1)

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		Button.SetVarAssoc ('PressedPortrait', i+1)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitButtonOnPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, PortraitButtonOnShiftPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnDropItemToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT, OnDropPortraitToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, PortraitButtonOnDrag)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, PortraitButtonOnMouseEnter)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, PortraitButtonOnMouseLeave)

		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 3, 3, 4, 4, 255, 255, 0, 255)

		ButtonHP = Window.GetControl (6 + i)
		ButtonHP.SetVarAssoc ('PressedPortraitHP', i+1)
		ButtonHP.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitButtonHPOnPress)

		portrait_hp_numeric[i] = 0

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = PortraitWindow

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		ButtonHP = Window.GetControl (6 + i)

		pic = GemRB.GetPlayerPortrait (i+1, 0)
		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			ButtonHP.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			continue

		#sel = GemRB.GameGetSelectedPCSingle () == i + 1
		Button.SetBAM (pic, 0, 0, -1)

		state = GemRB.GetPlayerStat (i+1, IE_STATE_ID)
		hp = GemRB.GetPlayerStat (i+1, IE_HITPOINTS)
		hp_max = GemRB.GetPlayerStat (i+1, IE_MAXHITPOINTS)

		if state & STATE_DEAD:
				cycle = 9
		elif state & STATE_HELPLESS:
				cycle = 8
		elif state & STATE_PETRIFIED:
				cycle = 7
		elif state & STATE_PANIC:
				cycle = 6
		elif state & STATE_POISONED:
				cycle = 2
		elif hp<hp_max/5:
			cycle = 4
		else:
			cycle = 0

		if cycle<6:
			Button.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED | IE_GUI_BUTTON_PLAYRANDOM|IE_GUI_BUTTON_DRAGGABLE|IE_GUI_BUTTON_MULTILINE, OP_SET)
		else:
			Button.SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED | IE_GUI_BUTTON_DRAGGABLE|IE_GUI_BUTTON_MULTILINE, OP_SET)
		Button.SetAnimation (pic, cycle)


		ButtonHP.SetFlags(IE_GUI_BUTTON_PICTURE, OP_SET)

		if hp_max<1:
			ratio = 0.0
		else:
			ratio = (hp + 0.0) / hp_max
			if ratio > 1.0: ratio = 1.0
		r = int (255 * (1.0 - ratio))
		g = int (255 * ratio)

		ButtonHP.SetText ("%d / %d" %(hp, hp_max))
		ButtonHP.SetTextColor (r, g, 0, False)
		ButtonHP.SetBAM ('FILLBAR', 0, 0, -1)
		ButtonHP.SetPictureClipping (ratio)

		if portrait_hp_numeric[i-1]:
			op = OP_NAND
		else:
			op = OP_OR

		ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)


		#if sel:
		#	Button.EnableBorder(FRAME_PC_SELECTED, 1)
		#else:
		#	Button.EnableBorder(FRAME_PC_SELECTED, 0)
	return

def PortraitButtonOnDrag ():
	global DraggedPortrait

	#they start from 1
	DraggedPortrait = GemRB.GetVar ("PressedPortrait")
	GemRB.DragItem (DraggedPortrait, -1, "")
	return

def PortraitButtonOnPress ():
	"""Selects the portrait individually."""

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	if GemRB.GameControlGetTargetMode() != TARGET_MODE_NONE:
		GemRB.ActOnPC (i)
		return

	if (not SelectionChangeHandler):
		if GemRB.GameIsPCSelected (i):
			GemRB.GameControlSetScreenFlags (SF_CENTERONACTOR, OP_OR)
		GemRB.GameSelectPC (i, True, SELECT_REPLACE)
	else:
		GemRB.GameSelectPCSingle (i)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def PortraitButtonOnShiftPress ():
	"""Handles selecting multiple portaits with shift."""

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	if (not SelectionChangeHandler):
		sel = GemRB.GameIsPCSelected (i)
		sel = not sel
		GemRB.GameSelectPC (i, sel)
	else:
		GemRB.GameSelectPCSingle (i)
		SelectionChanged ()
		RunSelectionChangeHandler ()
	return

def PortraitButtonHPOnPress ():
	Window = PortraitWindow

	i = GemRB.GetVar ('PressedPortraitHP')

	portrait_hp_numeric[i-1] = not portrait_hp_numeric[i-1]
	ButtonHP = Window.GetControl (5 + i)

	if portrait_hp_numeric[i-1]:
		op = OP_NAND
	else:
		op = OP_OR

	ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)
	return

# Run by Game class when selection was changed
def SelectionChanged ():
	# FIXME: hack. If defined, display single selection
	if (not SelectionChangeHandler):
		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
		if SelectionChangeMultiHandler:
			SelectionChangeMultiHandler ()
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, i + 1 == sel)
	import CommonWindow
	CommonWindow.CloseContainerWindow()
	return

def PortraitButtonOnMouseEnter ():
	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")

	if not i:
		return

	GemRB.GameControlSetLastActor( i )
	if GemRB.IsDraggingItem()==2:
		if DraggedPortrait != None:
			GemRB.SwapPCs (DraggedPortrait, i)
			#GemRB.SetVar ("PressedPortrait", DraggedPortrait)
			#possibly review why the other games do that ^^
			#it completely breaks the dragging in PST
			DraggedPortrait = i
			GemRB.SetTimedEvent (CheckDragging, 1)
		else:
			OnDropPortraitToPC()
		return

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i-1)
		Button.EnableBorder (FRAME_PC_TARGET, 1)
	return

def OnDropPortraitToPC ():
	GemRB.SetVar ("PressedPortrait",0)
	GemRB.DragItem (0, -1, "")
	DraggedPortrait = None
	return

def CheckDragging():
	"""Contains portrait dragging in case of mouse out-of-range."""

	global DraggedPortrait

	i = GemRB.GetVar ("PressedPortrait")
	if not i:
		GemRB.DragItem (0, -1, "")

	if GemRB.IsDraggingItem()!=2:
		DraggedPortrait = None
	return

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ("PressedPortrait")
	if not i:
		return

	Button = PortraitWindow.GetControl (i-1)
	Button.EnableBorder (FRAME_PC_TARGET, 0)
	GemRB.SetVar ("PressedPortrait", 0)
	GemRB.SetTimedEvent (CheckDragging, 1)
	return

def ActionStopPressed ():
	for i in GemRB.GetSelectedActors():
		GemRB.ClearActions (i)
	return

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK,GA_NO_DEAD|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK,GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_DEFEND,GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)
#FIXME: there is currently no way to use this  in pst
def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD|GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def DisableAnimatedWindows ():
	global ActionsWindow, OptionsWindow
	GemRB.SetVar ("PortraitWindow", -1)
	ActionsWindow = GUIClasses.GWindow( GemRB.GetVar ("ActionsWindow") )
	GemRB.SetVar ("ActionsWindow", -1)
	OptionsWindow = GUIClasses.GWindow( GemRB.GetVar ("OptionsWindow") )
	GemRB.SetVar ("OptionsWindow", -1)
	GemRB.GamePause (1,3)

def EnableAnimatedWindows ():
	GemRB.SetVar ("PortraitWindow", PortraitWindow.ID)
	GemRB.SetVar ("ActionsWindow", ActionsWindow.ID)
	GemRB.SetVar ("OptionsWindow", OptionsWindow.ID)
	GemRB.GamePause (0,3)

def SetItemButton (Window, Button, Slot, PressHandler, RightPressHandler):
	if Slot != None:
		Item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot['Flags'] & IE_INV_ITEM_IDENTIFIED
		#Button.SetVarAssoc ("LeftIndex", LeftTopIndex+i)
		#Button.SetSprites ('IVSLOT', 0,  0, 0, 0, 0)
		Button.SetItemIcon (Slot['ItemResRef'],0)

		if Item['MaxStackAmount'] > 1:
			Button.SetText (str (Slot['Usages0']))
		else:
			Button.SetText ('')


		if not identified or Item['ItemNameIdentified'] == -1:
			Button.SetTooltip (Item['ItemName'])
		else:
			Button.SetTooltip (Item['ItemNameIdentified'])

		#Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		#Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PressHandler)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, RightPressHandler)
		#Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, ShiftPressHandler)
		#Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, DragDropHandler)

	else:
		#Button.SetVarAssoc ("LeftIndex", -1)
		Button.SetItemIcon ('')
		Button.SetTooltip (4273)  # Ground Item
		Button.SetText ('')
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, None)
		#Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, None)
		#Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, None)

def OpenWaitForDiscWindow ():
	global DiscWindow
	#print "OpenWaitForDiscWindow"

	if DiscWindow:
		GemRB.HideGUI ()
		if DiscWindow:
			DiscWindow.Unload ()
		GemRB.SetVar ("OtherWindow", -1)
		# ...LoadWindowPack()
		EnableAnimatedWindows ()
		DiscWindow = None
		GemRB.UnhideGUI ()
		return

	try:
		GemRB.HideGUI ()
	except:
		pass

	GemRB.LoadWindowPack ("GUIID")
	DiscWindow = Window = GemRB.LoadWindow (0)
	GemRB.SetVar ("OtherWindow", Window.ID)
	label = DiscWindow.GetControl (0)

	disc_num = GemRB.GetVar ("WaitForDisc")
	#disc_path = GemRB.GetVar ("WaitForDiscPath")
	disc_path = 'XX:'

	text = GemRB.GetString (31483) + " " + str (disc_num) + " " + GemRB.GetString (31569) + " " + disc_path + "\n" + GemRB.GetString (49152)
	label.SetText (text)
	DisableAnimatedWindows ()

	# 31483 - Please place PS:T disc number
	# 31568 - Please place the PS:T DVD
	# 31569 - in drive
	# 31570 - Wrong disc in drive
	# 31571 - There is no disc in drive
	# 31578 - No disc could be found in drive. Please place Disc 1 in drive.
	# 49152 - To quit the game, press Alt-F4


	try:
		GemRB.UnhideGUI ()
	except:
		DiscWindow.SetVisible (WINDOW_VISIBLE)

def SetPSTGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	hours = (currentTime % 7200) / 300
	if hours < 12:
		ampm = "AM"
	else:
		ampm = "PM"
		hours -= 12
	minutes = (currentTime % 300) / 60

	GemRB.SetToken ('CLOCK_HOUR', str (hours))
	GemRB.SetToken ('CLOCK_MINUTE', '%02d' %minutes)
	GemRB.SetToken ('CLOCK_AMPM', ampm)

def UpdateClock():
	ActionsWindow = GemRB.LoadWindow(0)
	Button = ActionsWindow.GetControl (0)
	SetPSTGamedaysAndHourToken ()
	Button.SetTooltip (GemRB.GetString(65027))
	#this function does update the clock tip, but the core fails to display it
	
def UpdateActionsWindow():
	UpdateClock()
	#This at least ensures the clock gets a relatively regular update
	return

#this is an unused callback in PST
def EmptyControls ():
	return

def CheckLevelUp(pc):
	GemRB.SetVar ("CheckLevelUp"+str(pc), LUCommon.CanLevelUp (pc))

def ToggleAlwaysRun():
	GemRB.GameControlToggleAlwaysRun()

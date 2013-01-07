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
import GUIClasses ##this may not be absolutely necessary?
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

if GUICommon.GameIsPST():
  TimeWindow = None
  PortWindow = None
  MenuWindow = None
  MainWindow = None
  DiscWindow = None

PortraitWindow = None
OptionsWindow = None
ActionsWindow = None
CurrentWindow = None
DraggedPortrait = None
ActionBarControlOffset = 0


#if the GUIEnhancements bit is set this repositions a window from its original 640x480 position to somewhere saner
#returns false if the option is not set
#Ypos 0 = drop to bottom ; 1 = centre; 2 = top
#Xpos: todo if needed
def RepositionWindow(Window=0, Ypos=1): #set 0 for game conrols
	if(GemRB.GetVar("GUIEnhancements"))&GE_OVERRIDE_CHU_POSITIONS:
		
		if Window:
			screen_width = GemRB.GetSystemVariable (SV_WIDTH)
			screen_height = GemRB.GetSystemVariable (SV_HEIGHT)
		
			if screen_height == 600 and screen_width == 800:
				return #gemrb ships with GUIW08.CHU
		
			ofsx = (screen_width-640) / 2 #some handy offsets to centre controls
			ofsy = (screen_height-480) / 2
			position = Window.GetPos()
			height = 480 - position[1]
			if Ypos == 1:
				Window.SetPos(position[0]+ofsx, position[1]+ofsy)
			elif Ypos == 2:
				Window.SetPos(position[0]+ofsx, 0)
			else:
				Window.SetPos(position[0]+ofsx, screen_height-height)

		return True
		
	return False


OptionTip = { #dictionary to the stringrefs in each games dialog.tlk
'Inventory' : 41601,
'Map': 41625,
'Mage': 41624,
'Priest': 4709,
'Stats': 4707,
'Journal': 41623,
'Options' : 41626,
'Rest': 41628,
'Follow': 41647,
'Expand': 41660,
'AI' : 1,
'Game' : 1,
'Party' : 1
}

OptionControl = { #dictionary to the control indexes in the window (.CHU)
'Inventory' : 1,
'Map' : 2,
'Mage': 3,
'Priest': 7,
'Stats': 5,
'Journal': 6,
'Options' : 8,
'Rest': 9,
'Follow': 0, #pst
'Expand': 10, #pst
'AI': 4, #pst
'Game': 0, #not in pst
'Party' : 8 #not in pst
}

def SetupMenuWindowControls (Window, Gears=None, ReturnToGame=None): # gears/rtg not used in pst
	"""Sets up all of the basic control windows."""

	global OptionsWindow, ActionBarControlOffset

	OptionsWindow = Window
	# FIXME: add "(key)" to tooltips!

	if GUICommon.GameIsIWD2():
		ActionBarControlOffset = 6
		SetupIWD2WindowControls (Window, Gears, ReturnToGame)
		return

	if not GUICommon.GameIsPST(): ## pst lacks these two controls
		# Return to Game
		Button = Window.GetControl (OptionControl['Game'])
		Button.SetTooltip (OptionTip['Game'])
		Button.SetVarAssoc ("SelectedWindow", 0)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReturnToGame)
		Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)
		if GUICommon.GameIsBG1():
			# enabled BAM isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,28,16)
		if GUICommon.GameIsIWD1():
			# disabled/selected frame isn't present in .chu, defining it here
			Button.SetSprites ("GUILSOP", 0,16,17,16,16)
		# Party mgmt
		Button = Window.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: OpenPartyWindow
		if GUICommon.GameIsBG1() or GUICommon.GameIsBG2():
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		else:
			Button.SetTooltip (OptionTip['Party'])
	else: #pst has these two instead
		# (Un)Lock view on character
		Button = Window.GetControl (OptionControl['Follow'])
		Button.SetTooltip (OptionTip['Follow'])  # or 41648 Unlock ...
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OnLockViewPress)
		# AI
		Button = Window.GetControl (OptionControl['AI'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, AIPress)
		AIPress(toggle=0)


	# Map
	Button = Window.GetControl (OptionControl['Map'])
	Button.SetTooltip (OptionTip['Map'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Map'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMA.OpenMapWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,0,1,20,0)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,0,1,20,20)

	# Journal
	Button = Window.GetControl (OptionControl['Journal'])
	Button.SetTooltip (OptionTip['Journal'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Journal'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIJRNL.OpenJournalWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,4,5,22,4)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,4,5,22,22)

	# Inventory
	Button = Window.GetControl (OptionControl['Inventory'])
	Button.SetTooltip (OptionTip['Inventory'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Inventory'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.OpenInventoryWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,2,3,21,2)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,2,3,21,21)
	# Records
	Button = Window.GetControl (OptionControl['Stats'])
	Button.SetTooltip (OptionTip['Stats'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Stats'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIREC.OpenRecordsWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,6,7,23,6)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,6,7,23,23)

	# Mage
	Button = Window.GetControl (OptionControl['Mage'])
	Button.SetTooltip (OptionTip['Mage'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Mage'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMG.OpenMageWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,8,9,24,8)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,8,9,24,24)

	# Priest
	Button = Window.GetControl (OptionControl['Priest'])
	Button.SetTooltip (OptionTip['Priest'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Priest'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIPR.OpenPriestWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,10,11,25,10)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,10,11,25,25)

	# Options
	Button = Window.GetControl (OptionControl['Options'])
	Button.SetTooltip (OptionTip['Options'])
	Button.SetVarAssoc ("SelectedWindow", OptionControl['Options'])
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIOPT.OpenOptionsWindow)
	if GUICommon.GameIsBG1():
		Button.SetSprites ("GUILSOP", 0,12,13,26,12)
	if GUICommon.GameIsIWD1():
		Button.SetSprites ("GUILSOP", 0,12,13,26,26)
	# pause button
	if Gears:
		# Pendulum, gears, sun/moon dial (time)
		# FIXME: display all animations: CPEN, CGEAR, CDIAL
		if GUICommon.HasHOW(): # how doesn't have this in the right place
			pos = GemRB.GetSystemVariable (SV_HEIGHT)-71
			Window.CreateButton (9, 6, pos, 64, 71)
		Button = Window.GetControl (9)
		if GUICommon.GameIsBG2():
			Label = Button.CreateLabelOnButton (0x10000009, "NORMAL", 0)
			Label.SetAnimation ("CPEN")

		Button.SetAnimation ("CGEAR")
		if GUICommon.GameIsBG2():
			Button.SetBAM ("CDIAL", 0, 0)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ANIMATED|IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
		GUICommon.SetGamedaysAndHourToken()
		Button.SetTooltip(GemRB.GetString (16041))
		rb = 11
	else:
		rb = OptionControl['Rest']

	# Rest
	if Window.HasControl (rb):
		Button = Window.GetControl (rb)
		Button.SetTooltip (OptionTip['Rest'])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

	#MarkMenuButton (Window) 


	if PortraitWindow:
		UpdatePortraitWindow ()
	return

	# Message popup FIXME disable on non game screen...
	Button = Window.GetControl (OptionControl['Expand'])
	Button.SetTooltip (OptionTip['Expand'])  # or 41661 Close ...


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

def SetupActionsWindowControls (Window):
	global ActionsWindow

	ActionsWindow = Window
	# time button
	Button = Window.GetControl (0)
	Button.SetAnimation ("WMTIME")
	Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, GUICommon.GearsClicked)
	Button.SetEvent(IE_GUI_MOUSE_ENTER_BUTTON, UpdateClock)

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
	i = GemRB.GetVar ('PressedPortrait')

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

def StopAllOnPress ():
	for i in GemRB.GetSelectedActors():
		GemRB.ClearActions (i)
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

	if DraggedPortrait != None:
		GemRB.DragItem (0, -1, "")
		#this might not work
		GemRB.SwapPCs (DraggedPortrait, i)
		DraggedPortrait = None

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i-1)
		Button.EnableBorder (FRAME_PC_TARGET, 1)

def PortraitButtonOnMouseLeave ():
	i = GemRB.GetVar ("PressedPortrait")
	if not i:
		return

	if GemRB.IsDraggingItem ():
		Button = PortraitWindow.GetControl (i-1)
		Button.EnableBorder (FRAME_PC_TARGET, 0)
	return

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
	Button.SetTooltip (65027)

def UpdateActionsWindow():
	# pst doesn't need this, but it is one of the core callbacks, so it has to be defined
	return

def ActionStopPressed ():
	for i in range (PARTY_SIZE):
		if GemRB.GameIsPCSelected (i + 1):
			GemRB.ClearActions (i + 1)
	return

def ActionTalkPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_TALK,GA_NO_DEAD|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionAttackPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK,GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)

def ActionDefendPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_DEFEND,GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD|GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

#this is an unused callback in PST
def EmptyControls ():
	return

def CheckLevelUp(pc):
	GemRB.SetVar ("CheckLevelUp"+str(pc), LUCommon.CanLevelUp (pc))

def ToggleAlwaysRun():
	GemRB.GameControlToggleAlwaysRun()

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
'Inventory' : 16307,
'Map': 16310,
'Mage': 16309,
'Priest': 14930,
'Stats': 16306,
'Journal': 16308,
'Options' : 16311,
'Rest': 11942,
'Follow': 41647,
'Expand': 41660,
'AI' : 1,
'Game' : 16313,
'Party' : 16312
}

OptionControl = { #dictionary to the control indexes in the window (.CHU)
'Inventory' : 3,
'Map' : 1,
'Mage': 5,
'Priest': 6,
'Stats': 4,
'Journal': 2,
'Options' : 7,
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

	MarkMenuButton (Window)

	if PortraitWindow:
		UpdatePortraitWindow ()
	return

def SetupIWD2WindowControls (Window, Gears, ReturnToGame):
	"""Sets up all of the basic iwd2 control windows."""

	global OptionsWindow

	OptionsWindow = Window
	#Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, ReturnToGame)

	# Spellbook
	Button = Window.GetControl (4)
	Button.SetTooltip (16309)
	Button.SetVarAssoc ("SelectedWindow", 0)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUISPL.OpenSpellBookWindow)

	# Inventory
	Button = Window.GetControl (5)
	Button.SetTooltip (16307)
	Button.SetVarAssoc ("SelectedWindow", 1)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIINV.OpenInventoryWindow)

	# Journal
	Button = Window.GetControl (6)
	Button.SetTooltip (16308)
	Button.SetVarAssoc ("SelectedWindow", 2)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIJRNL.OpenJournalWindow)

	# Map
	Button = Window.GetControl (7)
	Button.SetTooltip (16310)
	Button.SetVarAssoc ("SelectedWindow", 3)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIMA.OpenMapWindow)

	# Records
	Button = Window.GetControl (8)
	Button.SetTooltip (16306)
	Button.SetVarAssoc ("SelectedWindow", 4)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIREC.OpenRecordsWindow)

	# Options
	Button = Window.GetControl (9)
	Button.SetTooltip (16311)
	Button.SetVarAssoc ("SelectedWindow", 7)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIOPT.OpenOptionsWindow)

	# Gears
	Button = Window.GetControl (10)
	Button.SetAnimation ("CGEAR")
	Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ANIMATED, OP_SET)
	Button.SetState (IE_GUI_BUTTON_LOCKED)

	if Gears:
		# Select All
		Button = Window.GetControl (11)
		Button.SetTooltip (10485)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectAllOnPress)
		# Rest
		Button = Window.GetControl (12)
		Button.SetTooltip (11942)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

		# Character Arbitration
		Button = Window.GetControl (13)
		Button.SetTooltip (16312)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, None) #TODO: CharacterWindow
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
	return

def PortraitPress ():
	"""Toggles the portraits pane """
	PP = GemRB.GetMessageWindowSize () & GS_PORTRAITPANE
	if PP:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_NAND)
	else:
		GemRB.GameSetScreenFlags (GS_PORTRAITPANE, OP_OR)
	return

def AIPress ():
	"""Toggles the party AI."""

	Button = PortraitWindow.GetControl (ActionBarControlOffset)
	AI = GemRB.GetMessageWindowSize () & GS_PARTYAI

	if AI:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_NAND)
		GemRB.SetVar ("AI", 0)
		Button.SetTooltip (15918)
	else:
		GemRB.GameSetScreenFlags (GS_PARTYAI, OP_OR)
		GemRB.SetVar ("AI", GS_PARTYAI)
		Button.SetTooltip (15917)

	#force redrawing, in case a hotkey triggered this function
	Button.SetVarAssoc ("AI", GS_PARTYAI)
	return

def EmptyControls ():
	Selected = GemRB.GetSelectedSize()
	if Selected==1:
		pc = GemRB.GameGetFirstSelectedActor ()
		#init spell list
		GemRB.SpellCast (pc, -1, 0)

	GemRB.SetVar ("ActionLevel", 0)
	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
		Button.SetPicture ("")
		Button.SetText ("")
		Button.SetActionIcon (globals(), -1)
	return

def SelectFormationPreset ():
	"""Choose the default formation."""
	GemRB.GameSetFormation (GemRB.GetVar ("Value"), GemRB.GetVar ("Formation") )
	GroupControls ()
	return

def SetupFormation ():
	"""Opens the formation selection section."""
	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		Button.SetFlags (IE_GUI_BUTTON_NORMAL, OP_SET)
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%i,0,0,-1)
		Button.SetVarAssoc ("Value", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectFormationPreset)
		Button.SetState (IE_GUI_BUTTON_UNPRESSED)
	return

def GroupControls ():
	"""Sections that control group actions."""
	GemRB.SetVar ("ActionLevel", 0)
	Button = CurrentWindow.GetControl (ActionBarControlOffset)
	if GUICommon.GameIsBG2():
		Button.SetActionIcon (globals(), 7) #talk icon
	else:
		Button.SetActionIcon (globals(), 14)#guard icon
	Button = CurrentWindow.GetControl (1+ActionBarControlOffset)
	Button.SetActionIcon (globals(), 15)
	Button = CurrentWindow.GetControl (2+ActionBarControlOffset)
	Button.SetActionIcon (globals(), 21)
	Button = CurrentWindow.GetControl (3+ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1)
	Button = CurrentWindow.GetControl (4+ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1)
	Button = CurrentWindow.GetControl (5+ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1)
	Button = CurrentWindow.GetControl (6+ActionBarControlOffset)
	Button.SetActionIcon (globals(), -1)
	GemRB.SetVar ("Formation", GemRB.GameGetFormation ())
	for i in range (5):
		Button = CurrentWindow.GetControl (7+ActionBarControlOffset+i)
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		idx = GemRB.GameGetFormation (i)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
		# kill the previous sprites or they show through
		Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
		Button.SetBAM ("FORM%x"%idx,0,0,-1)
		Button.SetVarAssoc ("Formation", i)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectFormation)
		Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, SetupFormation)
		Button.SetTooltip (4935, 8+i)
	return

def OpenActionsWindowControls (Window):
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

def SelectItemAbility():
	pc = GemRB.GameGetFirstSelectedActor ()
	slot = GemRB.GetVar ("Slot")
	ability = GemRB.GetVar ("Ability")
	GemRB.SetupQuickSlot (pc, 0, slot, ability)
	GemRB.SetVar ("ActionLevel", 0)
	return

def SelectQuiverSlot():
	pc = GemRB.GameGetFirstSelectedActor ()
	slot = GemRB.GetVar ("Slot")
	slot_item = GemRB.GetSlotItem (pc, slot)
	# HACK: implement SetEquippedAmmunition instead?
	if not GemRB.IsDraggingItem ():
		item = GemRB.GetItem (slot_item["ItemResRef"])
		GemRB.DragItem (pc, slot, item["ItemIcon"]) #, 0, 0)
		GemRB.DropDraggedItem (pc, slot)
	GemRB.SetVar ("ActionLevel", 0)
	return

def SetupItemAbilities(pc, slot):
	slot_item = GemRB.GetSlotItem(pc, slot)
	if not slot_item:
		# CHIV: Could configure empty quickslots from the game screen ala spells heres
		return

	item = GemRB.GetItem (slot_item["ItemResRef"])
	Tips = item["Tooltips"]

	# clear buttons here
	EmptyControls()

	# check A: whether ranged weapon and B: whether to bother at all
	ammotype = 0
	if item["Type"] == CommonTables.ItemType.GetRowIndex ("BOW"):
		ammotype = CommonTables.ItemType.GetRowIndex ("ARROW")
	elif item["Type"] == CommonTables.ItemType.GetRowIndex ("XBOW"):
		ammotype = CommonTables.ItemType.GetRowIndex ("BOLT")
	elif item["Type"] == CommonTables.ItemType.GetRowIndex ("SLING"):
		ammotype = CommonTables.ItemType.GetRowIndex ("BULLET")

	# FIXME: ammo does not preclude the item from also having abilities (eg. bg2:bow19)
	ammoSlotCount = 0
	if ammotype:
		ammoslots = GemRB.GetSlots(pc, SLOT_QUIVER, 1)
		currentammo = GemRB.GetEquippedAmmunition (pc)
		for i in range (12):
			Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
			if i < len(ammoslots):
				ammoslot = GemRB.GetSlotItem (pc, ammoslots[i])
				st = GemRB.GetSlotType (ammoslots[i])
				ammoitem = GemRB.GetItem (ammoslot['ItemResRef']) # needed to show the ammo count
				Tips = ammoitem["Tooltips"]
				# if this item is valid ammo and was really found in a quiver slot
				if ammoitem['Type'] == ammotype and st["Type"] == SLOT_QUIVER:
					ammoSlotCount += 1
					Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET)
					Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
					Button.SetItemIcon (ammoslot['ItemResRef'])
					Button.SetText (str(ammoslot["Usages0"]))
					Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectQuiverSlot)
					Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, SelectQuiverSlot)
					Button.SetVarAssoc ("Slot", ammoslots[i])
					Button.SetTooltip ("%s" % (GemRB.GetString (Tips[0])))
					print currentammo, i+1, ammoslots[i]
					if currentammo == ammoslots[i]: # this check is ok, but the results are unreliable?!
						Button.SetState (IE_GUI_BUTTON_SELECTED)

	# skip when there is only one choice
	if ammoSlotCount == 1:
		ammoSlotCount = 0

	# reset back to the main action bar if there are no extra headers or quivers
	reset = not ammoSlotCount
	# check for item abilities and skip the first that crops up (main header - nothing special)
	if (len(Tips) > 1):
		reset = False
		rmax = min(len(Tips) - 1, 12-ammoSlotCount)
		for i in range (rmax):
			Button = CurrentWindow.GetControl (i+ActionBarControlOffset+ammoSlotCount)
			Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NORMAL, OP_SET)
			Button.SetSprites ("GUIBTBUT",0,0,1,2,3)
			Button.SetItemIcon (slot_item['ItemResRef'], i+6)
			Button.SetText ("")
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, SelectItemAbility)
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, SelectItemAbility)
			Button.SetVarAssoc ("Ability", i)
			Button.SetState (IE_GUI_BUTTON_ENABLED)
			Button.SetTooltip ("F%d - %s"%(i,GemRB.GetString(Tips[i])) )

	if reset:
		GemRB.SetVar ("ActionLevel", 0)
		UpdateActionsWindow ()
	return

def SetupBookSelection ():
	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		Button.SetActionIcon (globals(), 50+i)
	return

#you can change this for custom skills, this is the original engine
skillbar=(ACT_STEALTH, ACT_SEARCH, ACT_THIEVING, ACT_WILDERNESS, ACT_TAMING, 100, 100, 100, 100, 100, 100, 100)
def SetupSkillSelection ():
	pc = GemRB.GameGetFirstSelectedActor ()
	CurrentWindow.SetupControls( globals(), pc, ActionBarControlOffset, skillbar)
	return

def UpdateActionsWindow ():
	"""Redraws the actions section of the window."""

	global CurrentWindow, OptionsWindow, PortraitWindow
	global level, TopIndex

	if GUICommon.GameIsIWD2():
		CurrentWindow = PortraitWindow
	else:
		CurrentWindow = ActionsWindow

	if CurrentWindow == -1:
		return

	if CurrentWindow == None:
		return

	#fully redraw the side panes to cover the actions window
	#do this only when there is no 'otherwindow'
	if GUICommon.GameIsIWD2():
		if GemRB.GetVar ("OtherWindow") != -1:
			return
	else:
		GUICommon.SetGamedaysAndHourToken()
		if OptionsWindow and OptionsWindow.HasControl (9):
			Button = OptionsWindow.GetControl (9)
			Button.SetTooltip (GemRB.GetString (16041)) # refetch the string, since the tokens changed
		elif ActionsWindow and ActionsWindow.HasControl (62):
			Button = ActionsWindow.GetControl (62)
			Button.SetTooltip (GemRB.GetString (16041))

		if GemRB.GetVar ("OtherWindow") == -1:
			if PortraitWindow:
				PortraitWindow.Invalidate ()
			if OptionsWindow:
				OptionsWindow.Invalidate ()

	Selected = GemRB.GetSelectedSize()

	#setting up the disabled button overlay (using the second border slot)
	for i in range (12):
		Button = CurrentWindow.GetControl (i+ActionBarControlOffset)
		if GUICommon.GameIsBG1():
			Button.SetBorder (0,6,6,4,4,0,254,0,255)
		Button.SetBorder (1, 0, 0, 0, 0, 50,30,10,120, 0, 1)
		Button.SetFont ("NUMBER")
		Button.SetText ("")
		Button.SetTooltip("")

	if Selected == 0:
		EmptyControls ()
		return
	if Selected > 1:
		GroupControls ()
		return

	#we are sure there is only one actor selected
	pc = GemRB.GameGetFirstSelectedActor ()

	level = GemRB.GetVar ("ActionLevel")
	TopIndex = GemRB.GetVar ("TopIndex")
	if level == 0:
		#this is based on class
		CurrentWindow.SetupControls (globals(), pc, ActionBarControlOffset)
	elif level == 1:
		CurrentWindow.SetupEquipmentIcons(globals(), pc, TopIndex, ActionBarControlOffset)
	elif level == 2: #spells
		if GUICommon.GameIsIWD2():
			type = 255
		else:
			type = 3
		GemRB.SetVar ("Type", type)
		Spellbook.SetupSpellIcons(CurrentWindow, type, TopIndex, ActionBarControlOffset)
	elif level == 3: #innates
		if GUICommon.GameIsIWD2():
			type = 256 + 1024
		else:
			type = 4
		GemRB.SetVar ("Type", type)
		Spellbook.SetupSpellIcons(CurrentWindow, type, TopIndex, ActionBarControlOffset)
	elif level == 4: #quick weapon/item ability selection
		SetupItemAbilities(pc, GemRB.GetVar("Slot") )
	elif level == 5: #all known mage spells
		GemRB.SetVar ("Type", -1)
		Spellbook.SetupSpellIcons(CurrentWindow, -1, TopIndex, ActionBarControlOffset)
	elif level == 6: # iwd2 skills
		SetupSkillSelection()
	elif level == 7: # quickspells, but with innates too
		if GUICommon.GameIsIWD2():
			type = 255 + 256 + 1024
		else:
			type = 7
		GemRB.SetVar ("Type", type)
		Spellbook.SetupSpellIcons(CurrentWindow, type, TopIndex, ActionBarControlOffset)
	elif level == 8: # shapes selection
		GemRB.SetVar ("Type", 1024)
		Spellbook.SetupSpellIcons(CurrentWindow, 1024, TopIndex, ActionBarControlOffset)
	elif level == 9: # songs selection
		GemRB.SetVar ("Type", 512)
		Spellbook.SetupSpellIcons(CurrentWindow, 512, TopIndex, ActionBarControlOffset)
	elif level == 10: # spellbook selection
		type = GemRB.GetVar ("Type")
		Spellbook.SetupSpellIcons(CurrentWindow, type, TopIndex, ActionBarControlOffset)
	elif level == 11: # spells from a 2da (fx_select_spell)
		if GUICommon.GameIsIWD2():
			type = 255
		else:
			type = 3
		GemRB.SetVar ("Type", type)
		Spellbook.SetupSpellIcons (CurrentWindow, type, TopIndex, ActionBarControlOffset)
	else:
		print "Invalid action level:", level
		GemRB.SetVar ("ActionLevel", 0)
	return

def ActionQWeaponPressed (which):
	"""Selects the given quickslot weapon if possible."""

	pc = GemRB.GameGetFirstSelectedActor ()
	qs = GemRB.GetEquippedQuickSlot (pc, 1)

	#38 is the magic slot
	if ((qs==which) or (qs==38)) and GemRB.GameControlGetTargetMode() != TARGET_MODE_ATTACK:
		GemRB.GameControlSetTargetMode (TARGET_MODE_ATTACK, GA_NO_DEAD|GA_NO_SELF|GA_NO_HIDDEN)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)
		GemRB.SetEquippedQuickSlot (pc, which, -1)

	CurrentWindow.SetupControls (globals(), pc, ActionBarControlOffset)
	UpdateActionsWindow ()
	return

def ActionQWeapon1Pressed ():
	ActionQWeaponPressed(0)

def ActionQWeapon2Pressed ():
	ActionQWeaponPressed(1)

def ActionQWeapon3Pressed ():
	ActionQWeaponPressed(2)

def ActionQWeapon4Pressed ():
	ActionQWeaponPressed(3)

def ActionQSpellPressed (which):
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.SpellCast (pc, -2, which)
	UpdateActionsWindow ()
	return

def ActionQSpell1Pressed ():
	ActionQSpellPressed(0)

def ActionQSpell2Pressed ():
	ActionQSpellPressed(1)

def ActionQSpell3Pressed ():
	ActionQSpellPressed(2)

def ActionQSpellRightPressed (which):
	GemRB.SetVar ("QSpell", which)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 7)
	UpdateActionsWindow ()
	return

def ActionQSpell1RightPressed ():
	ActionQSpellRightPressed(0)

def ActionQSpell2RightPressed ():
	ActionQSpellRightPressed(1)

def ActionQSpell3RightPressed ():
	ActionQSpellRightPressed(2)

# can't pass the globals dictionary from another module
def SetActionIconWorkaround(Button, action, function):
	Button.SetActionIcon (globals(), action, function)

#no check needed because the button wouldn't be drawn if illegal
def ActionLeftPressed ():
	"""Scrolls the actions window left.

	Used primarily for spell selection."""

	TopIndex = GemRB.GetVar ("TopIndex")
	if TopIndex>10:
		TopIndex -= 10
	else:
		TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

#no check needed because the button wouldn't be drawn if illegal
def ActionRightPressed ():
	"""Scrolls the action window right.

	Used primarily for spell selection."""

	pc = GemRB.GameGetFirstSelectedActor ()
	TopIndex = GemRB.GetVar ("TopIndex")
	Type = GemRB.GetVar ("Type")
	print "Type:", Type
	#Type is a bitfield if there is no level given
	#This is to make sure cleric/mages get all spells listed
	if GemRB.GetVar ("ActionLevel") == 5:
		if Type == 3:
			Max = len(Spellbook.GetKnownSpells (pc, IE_SPELL_TYPE_PRIEST) + Spellbook.GetKnownSpells (pc, IE_SPELL_TYPE_WIZARD))
		else:
			Max = GemRB.GetKnownSpellsCount (pc, Type, -1) # this can handle only one type at a time
	else:
		Max = GemRB.GetMemorizedSpellsCount(pc, Type, -1, 1)
	print "Max:",Max
	TopIndex += 10
	if TopIndex > Max - 10:
		if Max>10:
			if TopIndex > Max:
				TopIndex = Max - 10
		else:
			TopIndex = 0
	GemRB.SetVar ("TopIndex", TopIndex)
	UpdateActionsWindow ()
	return

def ActionMeleePressed ():
	""" switches to the most damaging melee weapon"""
	#get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipMostDamagingMelee()", pc)
	return

def ActionRangePressed ():
	""" switches to the most damaging ranged weapon"""
	#get the party Index
	pc = GemRB.GameGetFirstSelectedPC ()
	GemRB.ExecuteString("EquipRanged()", pc)
	return

def ActionShapeChangePressed ():
	GemRB.SetVar ("ActionLevel", 8)
	UpdateActionsWindow ()
	return

def ActionBardSongRightPressed ():
	"""Selects a bardsong."""
	GemRB.SetVar ("ActionLevel", 9)
	UpdateActionsWindow ()
	return

def ActionBardSongPressed ():
	"""Toggles the battle song."""

	##FIXME: check if the actor can actually switch to this state
	#get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_BATTLESONG)
	GemRB.PlaySound ("act_01")
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionSearchPressed ():
	"""Toggles detect traps."""

	##FIXME: check if the actor can actually switch to this state
	#get the global ID
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_DETECTTRAPS)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionStealthPressed ():
	"""Toggles stealth."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_STEALTH)
	GemRB.PlaySound ("act_07")
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionTurnPressed ():
	"""Toggles turn undead."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetModalState (pc, MS_TURNUNDEAD)
	GemRB.PlaySound ("act_06")
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionTamingPressed ():
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionWildernessPressed ():
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

def ActionUseItemPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 1)
	UpdateActionsWindow ()
	return

def ActionCastPressed ():
	"""Opens the spell choice scrollbar."""
	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 2)
	UpdateActionsWindow ()
	return

def ActionQItemPressed (action):
	"""Uses the given quick item."""
	pc = GemRB.GameGetFirstSelectedActor ()
	#quick slot
	GemRB.UseItem (pc, -2, action, -1)
	return

def ActionQItem1Pressed ():
	ActionQItemPressed (ACT_QSLOT1)
	return

def ActionQItem2Pressed ():
	ActionQItemPressed (ACT_QSLOT2)
	return

def ActionQItem3Pressed ():
	ActionQItemPressed (ACT_QSLOT3)
	return

def ActionQItem4Pressed ():
	ActionQItemPressed (ACT_QSLOT4)
	return

def ActionQItem5Pressed ():
	ActionQItemPressed (ACT_QSLOT5)
	return

def ActionQItemRightPressed (action):
	"""Selects the used ability of the quick item."""
	pc = GemRB.GameGetFirstSelectedActor ()
	GemRB.SetVar ("Slot", action)
	GemRB.SetVar ("ActionLevel", 4)
	UpdateActionsWindow ()
	return

def ActionQItem1RightPressed ():
	ActionQItemRightPressed (19)

def ActionQItem2RightPressed ():
	ActionQItemRightPressed (20)

def ActionQItem3RightPressed ():
	ActionQItemRightPressed (21)

def ActionQItem4RightPressed ():
	ActionQItemRightPressed (22)

def ActionQItem5RightPressed ():
	ActionQItemRightPressed (23)

def ActionQWeapon1RightPressed ():
	ActionQItemRightPressed (10)

def ActionQWeapon2RightPressed ():
	ActionQItemRightPressed (11)

def ActionQWeapon3RightPressed ():
	ActionQItemRightPressed (12)

def ActionQWeapon4RightPressed ():
	ActionQItemRightPressed (13)

def ActionInnatePressed ():
	"""Opens the innate spell scrollbar."""
	GemRB.SetVar ("QSpell", -1)
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 3)
	UpdateActionsWindow ()
	return

def ActionSkillsPressed ():
	GemRB.SetVar ("TopIndex", 0)
	GemRB.SetVar ("ActionLevel", 6)
	UpdateActionsWindow ()
	return

def TypeSpellPressed (type):
	GemRB.SetVar ("Type", 1<<type)
	GemRB.SetVar ("ActionLevel", 10)
	UpdateActionsWindow ()
	return

def ActionBardSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_BARD)
	return

def ActionClericSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_CLERIC)
	return

def ActionDruidSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_DRUID)
	return

def ActionPaladinSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_PALADIN)
	return

def ActionRangerSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_RANGER)
	return

def ActionSorcererSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_SORCEROR)
	return

def ActionWizardSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_WIZARD)
	return

def ActionDomainSpellPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_DOMAIN)
	return

def ActionWildShapesPressed ():
	TypeSpellPressed(IE_IWD2_SPELL_SHAPE)
	return

# This is the endpoint for spellcasting, finally calling SpellCast. This always happens at least
# twice though, the second time to reset the action bar (more if wild magic or subspell selection is involved).
# Spell and Type (spellbook type) are set during the spell bar construction/use, which is in turn
# affected by ActionLevel (see UpdateActionsWindow of this module).
# Keep in mind, that the core resets Type and/or ActionLevel in the case of subspells (fx_select_spell).
def SpellPressed ():
	"""Prepares a spell to be cast."""

	pc = GemRB.GameGetFirstSelectedActor ()

	Spell = GemRB.GetVar ("Spell")
	Type = GemRB.GetVar ("Type")

	if Type == 1024:
		#SelectBardSong(Spell)
		ActionBardSongPressed()
		return

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	if Type != -1:
		Type = Spell // 1000
	Spell = Spell % 1000
	slot = GemRB.GetVar ("QSpell")
	if slot>=0:
		#setup quickspell slot
		#if spell has no target, return
		#otherwise continue with casting
		Target = GemRB.SetupQuickSpell (pc, slot, Spell, Type)
		#sabotage the immediate casting of self targeting spells
		if Target == 5 or Target == 7:
			Type = -1
			GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

	if Type==-1:
		GemRB.SetVar ("ActionLevel", 0)
		GemRB.SetVar("Type", 0)
	GemRB.SpellCast (pc, Type, Spell)
	if GemRB.GetVar ("Type")!=-1:
		GemRB.SetVar ("ActionLevel", 0)
		#init spell list
		GemRB.SpellCast (pc, -1, 0)
	GemRB.SetVar ("TopIndex", 0)
	UpdateActionsWindow ()
	return

def EquipmentPressed ():
	pc = GemRB.GameGetFirstSelectedActor ()

	GemRB.GameControlSetTargetMode (TARGET_MODE_CAST)
	Item = GemRB.GetVar ("Equipment")
	#equipment index
	GemRB.UseItem (pc, -1, Item, -1)
	GemRB.SetVar ("ActionLevel", 0)
	UpdateActionsWindow ()
	return

# NOTE: the following 4 functions are only used in iwd2
def GetActorRaceTitle (actor):
	RaceID = GemRB.GetPlayerStat (actor, IE_SUBRACE)
	if RaceID:
		RaceID += GemRB.GetPlayerStat (actor, IE_RACE)<<16
	else:
		RaceID = GemRB.GetPlayerStat (actor, IE_RACE)
	row = CommonTables.Races.FindValue (3, RaceID )
	RaceTitle = CommonTables.Races.GetValue (row, 2)
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
		if BaseClass == ClassID and Kit & CommonTables.Classes.GetValue (RowName, "ID"):
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
	if (not SelectionChangeHandler) and handler and (not GUICommon.NextWindowFn):
		sel = GemRB.GameGetFirstSelectedPC ()
		if not sel:
			sel = 1
		GemRB.GameSelectPCSingle (sel)

	SelectionChangeHandler = handler

	# redraw selection on change main selection | single selection
	SelectionChanged ()
	return

def RunSelectionChangeHandler ():
	if SelectionChangeHandler:
		SelectionChangeHandler ()
	return

def OpenPortraitWindow (needcontrols=0):
	global PortraitWindow

	#take care, this window is different in how/iwd
	if GUICommon.HasHOW() and needcontrols:
		PortraitWindow = Window = GemRB.LoadWindow (26)
	else:
		PortraitWindow = Window = GemRB.LoadWindow (1)

	if needcontrols:
		# 1280 and higher don't have this control
		if Window.HasControl (8):
			Button=Window.GetControl (8)
			if GUICommon.GameIsIWD():
				# Rest (iwd)
				Button.SetTooltip (11942)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)
			else:
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MinimizePortraits)
		else:
			if GUICommon.HasHOW():
				# Rest (how)
				pos = GemRB.GetSystemVariable (SV_HEIGHT) - 37
				Window.CreateButton (8, 6, pos, 55, 37)
				Button = Window.GetControl (8)
				Button.SetSprites ("GUIRSBUT", 0,0,1,0,0)
				Button.SetTooltip (11942)
				Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

				pos = pos - 37
				Window.CreateButton (6, 6, pos, 27, 36)

		# AI
		Button = Window.GetControl (6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		Button.SetSprites ("GUIBTACT", 0, 46, 47, 48, 49)
		GSFlags = GemRB.GetMessageWindowSize ()&GS_PARTYAI

		GemRB.SetVar ("AI", GSFlags)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, AIPress)
		Button.SetFlags (IE_GUI_BUTTON_CHECKBOX, OP_OR)
		Button.SetVarAssoc ("AI", GS_PARTYAI)
		if GSFlags:
			Button.SetTooltip (15917)
		else:
			Button.SetTooltip (15918)

		#Select All
		if GUICommon.HasHOW():
			Window.CreateButton (7, 33, pos, 27, 36)
			Button = Window.GetControl (7)
			Button.SetSprites ("GUIBTACT", 0, 50, 51, 50, 51)
		else:
			Button = Window.GetControl (7)
		Button.SetTooltip (10485)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.SelectAllOnPress)
	else:
		# Rest
		if Window.HasControl(6) and not GUICommon.GameIsIWD2():
			Button = Window.GetControl (6)
			Button.SetTooltip (11942)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommon.RestPress)

	for i in range (PARTY_SIZE):
		Button = Window.GetControl (i)
		if GUICommon.GameIsIWD1() or GUICommon.GameIsIWD2():
			Button.SetFont ("STATES")
			# label for status flags (dialog, store, level up)
			Button.CreateLabelOnButton(200 + i, "STATES", IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_RIGHT) #level up icon is on the right
		else:
			Button.SetFont ("STATES2")
			# label for status flags (dialog, store, level up)
			Button.CreateLabelOnButton(200 + i, "STATES2", IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_RIGHT) #level up icon is on the right

		Button.SetVarAssoc ("PressedPortrait", i+1)

		if needcontrols or GUICommon.GameIsIWD2():
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, GUIINV.OpenInventoryWindowClick)
		else:
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, PortraitButtonOnPress)

		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitButtonOnPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_SHIFT_PRESS, PortraitButtonOnShiftPress)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP, InventoryCommon.OnDropItemToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG_DROP_PORTRAIT, OnDropPortraitToPC)
		Button.SetEvent (IE_GUI_BUTTON_ON_DRAG, PortraitButtonOnDrag)
		Button.SetEvent (IE_GUI_MOUSE_ENTER_BUTTON, PortraitButtonOnMouseEnter)
		Button.SetEvent (IE_GUI_MOUSE_LEAVE_BUTTON, PortraitButtonOnMouseLeave)

		Label = Window.GetControl(200 + i)
		if GUICommon.GameIsIWD1():
			# overlay a label, so we can display the hp with the correct font. Regular button label
			#   is used by effect icons
			Button.CreateLabelOnButton(100+i, "NUMFONT", IE_FONT_ALIGN_TOP|IE_FONT_ALIGN_LEFT)
			HPLabel = Window.GetControl (100+i)
			HPLabel.SetUseRGB (True)

		# unlike other buttons, this one lacks extra frames for a selection effect
		# so we create it and shift it to cover the grooves of the image
		# except iwd2's second frame already has it incorporated (but we miscolor it)
		if GUICommon.GameIsIWD2():
			Button.SetBorder (FRAME_PC_SELECTED, 0, 0, 0, 0, 0, 255, 0, 255)
		else:
			Button.SetBorder (FRAME_PC_SELECTED, 4, 3, 4, 3, 0, 255, 0, 255)
		Button.SetBorder (FRAME_PC_TARGET, 2, 2, 3, 3, 255, 255, 0, 255)

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = PortraitWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")

	for portid in range (PARTY_SIZE):
		Button = Window.GetControl (portid)
		pic = GemRB.GetPlayerPortrait (portid+1, 1)
		if Inventory and pc != portid+1:
			pic = None

		if pic and GemRB.GetPlayerStat(portid+1, IE_STATE_ID) & STATE_DEAD:
			import GUISTORE
			# dead pcs are hidden in all stores but temples
			if GUISTORE.StoreWindow and not GUISTORE.StoreHealWindow:
				pic = None

		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue

		portraitFlags = IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_HORIZONTAL | IE_GUI_BUTTON_ALIGN_LEFT | \
						IE_GUI_BUTTON_DRAGGABLE | IE_GUI_BUTTON_MULTILINE | IE_GUI_BUTTON_ALIGN_BOTTOM
		if GUICommon.GameIsIWD2():
			Button.SetEvent (IE_GUI_BUTTON_ON_RIGHT_PRESS, GUIINV.OpenInventoryWindowClick)
			Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitButtonOnPress)

		Button.SetFlags (portraitFlags, OP_SET)

		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetPicture (pic, "NOPORTSM")
		ratio_str, color = GUICommon.SetupDamageInfo (portid+1, Button, Window)

		# character - 1 == bam cycle
		talk = store = flag = blank = ""
		if GUICommon.GameIsBG2():
			# as far as I can tell only BG2 has icons for talk or store
			flag = blank = chr(238)
			talk = 154 # dialog icon
			store = 155 # shopping icon

			if pc==portid+1:
				if GemRB.GetStore()!=None:
					flag = chr(store)
			# talk icon
			if GemRB.GameGetSelectedPCSingle(1)==portid+1:
				flag = chr(talk)

		if LUCommon.CanLevelUp (portid+1):
			flag = flag + blank + chr(255)
		elif GUICommon.GameIsIWD1():
			HPLabel = Window.GetControl (100+portid)
			HPLabel.SetText (ratio_str)
			HPLabel.SetTextColor (*color)

		#add effects on the portrait
		effects = GemRB.GetPlayerStates (portid+1)

		numCols = 4 if GUICommon.GameIsIWD2() else 3
		numEffects = len(effects)

		states = ""
		# calculate the partial row
		idx = numEffects % numCols
		states = effects[0:idx] + "\n"

		for x in range(idx, numEffects): # now do any rows that are full
			states = states + effects[x]
			if (x - idx) % numCols == numCols - 1:
				states = states + "\n"

		FlagLabel = Window.GetControl(200 + portid)
		if flag != blank:
			FlagLabel.SetText(flag)
		else:
			FlagLabel.SetText("")
		Button.SetText(states)
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

def SelectionChanged ():
	"""Ran by the Game class when a PC selection is changed."""

	global PortraitWindow

	if not PortraitWindow:
		return

	GemRB.SetVar ("ActionLevel", 0)
	if (not SelectionChangeHandler):
		UpdateActionsWindow ()
		for i in range (PARTY_SIZE):
			Button = PortraitWindow.GetControl (i)
			Button.EnableBorder (FRAME_PC_SELECTED, GemRB.GameIsPCSelected (i + 1))
	else:
		sel = GemRB.GameGetSelectedPCSingle ()

		#update mage school
		GemRB.SetVar ("MAGESCHOOL", 0)
		Kit = GUICommon.GetKitIndex (sel)
		if Kit and CommonTables.KitList.GetValue (Kit, 7) == 1:
			MageTable = GemRB.LoadTable ("magesch")
			GemRB.SetVar ("MAGESCHOOL", MageTable.FindValue (3, CommonTables.KitList.GetValue (Kit, 6) ) )

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
			GemRB.SetVar ("PressedPortrait", DraggedPortrait)
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

def ActionThievingPressed ():
	GemRB.GameControlSetTargetMode (TARGET_MODE_PICK, GA_NO_DEAD|GA_NO_SELF|GA_NO_ENEMY|GA_NO_HIDDEN)

def MinimizePortraits():
	GemRB.GameSetScreenFlags(GS_PORTRAITPANE, OP_OR)

def OpenWaitForDiscWindow ():
	global DiscWindow

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

def CheckLevelUp(pc):
	GemRB.SetVar ("CheckLevelUp"+str(pc), LUCommon.CanLevelUp (pc))

def HideInterface():
	GemRB.GameSetScreenFlags (GS_HIDEGUI, OP_XOR)
	return

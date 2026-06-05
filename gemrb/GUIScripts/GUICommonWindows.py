# SPDX-FileCopyrightText: 2003, 2004 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# GUICommonWindows.py - shared code for managing top windows and more
###################################################

import GemRB
from GUIDefines import *
from ie_stats import *
from ie_restype import RES_2DA
import Clock
import CommonTables
import CommonWindow
import GameCheck
import GUICommon
import InventoryCommon

DiscWindow = None

# NOTE: the following two features are only used in pst
# which=INVENTORY|STATS|FMENU
def GetActorPortrait (actor, which):
	# GemRB.GetPlayerPortrait just returns the stored portrait

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
	RaceName = GUICommon.GetRaceRowName (actor)
	RaceTitle = CommonTables.Races.GetValue (RaceName, "UPPERCASE", GTV_REF)
	return RaceTitle

# NOTE: this function is called with the primary classes
def GetKitIndex (actor, ClassIndex):
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)

	KitIndex = -1
	ClassName = CommonTables.ClassText.GetRowName (ClassIndex)
	ClassID = CommonTables.ClassText.GetValue (ClassName, "CLASSID")
	# skip the primary classes
	# start at the first original kit - in iwd2 both classes and kits are in the same table
	KitOffset = CommonTables.Classes.FindValue ("CLASS", 7)
	for ci in range (KitOffset, CommonTables.Classes.GetRowCount ()):
		RowName = CommonTables.Classes.GetRowName (ci)
		BaseClass = CommonTables.Classes.GetValue (RowName, "CLASS")
		if BaseClass == ClassID and Kit & CommonTables.ClassText.GetValue (RowName, "CLASSID", GTV_INT):
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
	ClassTitle = CommonTables.ClassText.GetValue (ClassName, "LOWER")

	if ClassTitle == "*":
		return 0
	return ClassTitle

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
		Clock.UpdateClock()

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

		Clock.UpdateClock()

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
		Clock.UpdateClock ()
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
	import MenuWindow
	import Container

	# FIXME: hack. If defined, display single selection
	ActionsWindow.SetActionLevel (UAW_STANDARD)
	if (not SelectionChangeHandler):
		ActionsWindow.UpdateActionsWindow ()
	else:
		pc = GemRB.GameGetSelectedPCSingle();
		GUICommon.UpdateMageSchool (pc)

	UpdatePortraitWindow()
	Container.CloseContainerWindow()
	if SelectionChangeHandler:
		SelectionChangeHandler ()
	MenuWindow.UpdateMenuWindowControls ()
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

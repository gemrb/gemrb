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

import GemRB
import GUICommon

from GUIDefines import *
from GUICommonWindows import *

StatesFont = "STATES2"
if GameCheck.IsIWD1() or GameCheck.IsIWD2():
	StatesFont = "STATES"

ScreenHeight = GemRB.GetSystemVariable (SV_HEIGHT)

# returns buttons and a numerical index
# does nothing new in iwd2 due to layout
# in the rest, it will enable extra button generation for higher resolutions
# Mode determines arrangement direction, horizontal being for party reform and potentially save/load
def GetPortraitButtonPairs (Window, ExtraSlots=0, Mode="vertical"):
	pairs = {}

	if not Window:
		return pairs

	oldSlotCount = 6 + ExtraSlots

	for i in range(min(oldSlotCount, MAX_PARTY_SIZE + ExtraSlots)): # the default chu/game limit or less
		btn = Window.GetControl(i)
		btn.SetHotKey(chr(ord('1') + i), 0, True)
		pairs[i] = btn

	# nothing left to do
	PartySize = GemRB.GetPartySize ()
	if PartySize <= oldSlotCount:
		return pairs

	if GameCheck.IsIWD2():
		# set Mode = "horizontal" once we can create enough space
		GemRB.Log(LOG_ERROR, "GetPortraitButtonPairs", "Parties larger than 6 are currently not supported in IWD2! Using 6 ...")
		return pairs

	# GUIWORLD doesn't have a separate portraits window, so we need to skip
	# all this magic when reforming an overflowing party
	if PartySize > MAX_PARTY_SIZE:
		return pairs

	# generate new buttons by copying from existing ones
	firstButton = pairs[0]
	firstRect = firstButton.GetFrame ()
	buttonHeight = firstRect["h"]
	buttonWidth = firstRect["w"]
	xOffset = firstRect["x"]
	yOffset = firstRect["y"]
	windowRect = Window.GetFrame()
	windowHeight = windowRect["h"]
	windowWidth = windowRect["w"]
	limit = limitStep = 0
	scale = 0
	portraitGap = 0
	if Mode ==  "horizontal":
		xOffset += 3*buttonWidth  # width of other controls in party reform; we'll draw on the other side (at least in guiw8, guiw10 has no need for this)
		maxWidth = windowWidth - xOffset
		limit = maxWidth
		limitStep = buttonWidth
	else:
		# reduce it by existing slots + 0 slots in framed views (eg. inventory) and
		# 1 in main game control (for spacing and any other controls below (ai/select all in bg2))
		maxHeight = windowHeight - buttonHeight * PartySize - buttonHeight // 2
		if windowHeight != ScreenHeight:
			maxHeight += buttonHeight // 2
		limit = maxHeight
		# for framed views, limited to 6, we downscale the buttons to fit, clipping their portraits
		if maxHeight < buttonHeight:
			unused = -40 if GameCheck.IsBG1() else 20 # remaining unused space below the portraits
			scale = 1
			portraitGap = buttonHeight
			buttonHeight = (windowHeight + unused) // PartySize
			if portraitGap == buttonHeight:
				scale = 0 # ensure idempotence, eg. when this gets called on click
			portraitGap = portraitGap - buttonHeight - 2 # 2 for a quasi border
			limit = windowHeight - buttonHeight * 6 + unused
		limitStep = buttonHeight

	for i in range(len(pairs), PartySize):
		if limitStep > limit:
			raise SystemExit("Not enough window space for so many party members (portraits), bailing out! %d vs width/height of %d/%d" %(limit, buttonWidth, buttonHeight))
		nextID = 1000 + i
		control = Window.GetControl (nextID)
		if control:
			pairs[i] = control
			continue
		if Mode ==  "horizontal":
			Window.CreateButton (nextID, xOffset+i*buttonWidth, yOffset, buttonWidth, buttonHeight)
		else:
			# vertical
			Window.CreateButton (nextID, xOffset, i*buttonHeight+yOffset+i*2*scale, buttonWidth, buttonHeight)

		button = Window.GetControl (nextID)
		button.SetSprites ("GUIRSPOR", 0, 0, 1, 0, 0)
		button.SetVarAssoc ("portrait", i + 1)
		SetupButtonBorders (Window, button, i)
		button.SetFont (StatesFont)
		button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)

		button.OnRightPress (OpenInventoryWindowClick)
		button.OnPress (PortraitButtonOnPress)
		button.OnShiftPress (PortraitButtonOnShiftPress)
		button.SetAction (PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		button.SetAction (ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		button.SetAction (ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)

		pairs[i] = button
		limit -= limitStep

	# move the buttons back up, to combine the freed space
	if scale:
		for i in range(PartySize - 1):
			button = pairs[i]
			button.SetSize (buttonWidth, buttonHeight)
			if i == 0:
				continue # don't move the first portrait
			rect = button.GetFrame ()
			x = rect["x"]
			y = rect["y"]
			button.SetPos (x, y-portraitGap*i)

	return pairs

def AddStatusFlagLabel (Window, Button, i):
	label = Window.GetControl (200 + i)
	if label:
		return label

	# label for status flags (dialog, store, level up)
	align = IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_RIGHT | IE_FONT_SINGLE_LINE
	if GameCheck.IsBG2 ():
		align = IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_CENTER | IE_FONT_SINGLE_LINE
	label = Button.CreateLabel (200 + i, StatesFont, "", align) #level up icon is on the right
	label.SetFrame (Button.GetInsetFrame (4))
	return label

# overlay a label, so we can display the hp with the correct font. Regular button label
#   is used by effect icons
def AddHPLabel (Window, Button, i):
	label = Window.GetControl (100 + i)
	if label:
		return label

	label = Button.CreateLabel (100 + i, "NUMFONT", "", IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_LEFT | IE_FONT_SINGLE_LINE)
	return label

def SetupButtonBorders (Window, Button, i):
	# unlike other buttons, this one lacks extra frames for a selection effect
	# so we create it and shift it to cover the grooves of the image
	# except iwd2's second frame already has it incorporated (but we miscolor it)
	yellow = {'r' : 255, 'g' : 255, 'b' : 0, 'a' : 255}
	green = {'r' : 0, 'g' : 255, 'b' : 0, 'a' : 255}
	if GameCheck.IsIWD2 ():
		Button.SetBorder (FRAME_PC_SELECTED, green)
		Button.SetBorder (FRAME_PC_TARGET, yellow, 0, 0, Button.GetInsetFrame (2, 2, 3, 3))
	else:
		Button.SetBorder (FRAME_PC_SELECTED, green, 0, 0, Button.GetInsetFrame (4, 3, 4, 3))
		Button.SetBorder (FRAME_PC_TARGET, yellow, 0, 0, Button.GetInsetFrame (2, 2, 3, 3))

def OpenPortraitWindow (needcontrols=0, pos=WINDOW_RIGHT|WINDOW_VCENTER):
	#take care, this window is different in how/iwd
	if GameCheck.HasHOW() and needcontrols:
		Window = GemRB.LoadWindow (26, GUICommon.GetWindowPack(), pos)
	else:
		Window = GemRB.LoadWindow (1, GUICommon.GetWindowPack(), pos)

	Window.AddAlias("PORTWIN")
	Window.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	if needcontrols:
		# 1280 and higher don't have this control
		Button = Window.GetControl (8)
		if Button:
			if GameCheck.IsIWD():
				# Rest (iwd)
				Button.SetTooltip (11942)
				Button.OnPress (RestPress)
			else:
				Button.OnPress (MinimizePortraits)
		else:
			if GameCheck.HasHOW():
				# Rest (how)
				pos = Window.GetFrame()["h"] - 37
				Button = Window.CreateButton (8, 6, pos, 55, 37)
				Button.SetSprites ("GUIRSBUT", 0,0,1,0,0)
				Button.SetTooltip (11942)
				Button.OnPress (RestPress)

				pos = pos - 37
				Window.CreateButton (6, 6, pos, 27, 36)

		# AI
		Button = Window.GetControl (6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		Button.SetSprites ("GUIBTACT", 0, 46, 47, 48, 49)
		InitOptionButton(Window, 'Toggle_AI', AIPress)
		AIPress(0) #this initialises the state and tooltip

		#Select All
		if GameCheck.HasHOW():
			Button = Window.CreateButton (7, 33, pos, 27, 36)
			Button.SetSprites ("GUIBTACT", 0, 50, 51, 50, 51)
		else:
			Button = Window.GetControl (7)
		Button.SetTooltip (10485)
		Button.OnPress (GUICommon.SelectAllOnPress)
	else:
		# Rest
		if not GameCheck.IsIWD2():
			Button = Window.GetControl (6)
			if Button:
				Button.SetTooltip (11942)
				Button.OnPress (RestPress)

	PortraitButtons = GetPortraitButtonPairs (Window)
	for i, Button in PortraitButtons.items():
		pcID = i + 1

		Button.SetVarAssoc("portrait", pcID)
		Button.SetHotKey(chr(ord('1') + i), 0, True)
		Button.SetFont (StatesFont)
		AddStatusFlagLabel (Window, Button, i)

		if needcontrols or GameCheck.IsIWD2():
			Button.OnRightPress (OpenInventoryWindowClick)
		else:
			Button.OnRightPress (PortraitButtonOnPress)

		Button.OnPress (PortraitButtonOnPress)
		Button.OnShiftPress (PortraitButtonOnShiftPress)
		Button.SetAction (PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		Button.SetAction (ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		Button.SetAction (ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)

		AddHPLabel (Window, Button, i)
		SetupButtonBorders (Window, Button, i)

	UpdatePortraitWindow ()
	SelectionChanged ()
	return Window

def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = GemRB.GetView("PORTWIN")
	Window.Focus(None)

	pc = GemRB.GameGetSelectedPCSingle ()
	Inventory = GemRB.GetVar ("Inventory")
	GSFlags = GemRB.GetGUIFlags()

	PortraitButtons = GetPortraitButtonPairs (Window)
	for i, Button in PortraitButtons.items():
		pcID = i + 1
		if (pcID <= GemRB.GetPartySize()):
			Button.SetAction(lambda btn, pc=pcID: GemRB.GameControlLocateActor(pc), IE_ACT_MOUSE_ENTER);
			Button.SetAction(lambda: GemRB.GameControlLocateActor(-1), IE_ACT_MOUSE_LEAVE);

		Portrait = GemRB.GetPlayerPortrait (pcID, 1)
		Hide = False
		if Inventory and pc != pcID:
			Hide = True

		if Portrait and GemRB.GetPlayerStat(pcID, IE_STATE_ID) & STATE_DEAD:
			import GUISTORE
			# dead pcs are hidden in all stores but temples
			if GUISTORE.StoreWindow and not GUISTORE.StoreHealWindow:
				Hide = True

		if Hide or not Portrait:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_DISABLED)
			Button.SetText ("")
			Button.SetTooltip ("")
			continue

		portraitFlags = IE_GUI_BUTTON_PORTRAIT | IE_GUI_BUTTON_HORIZONTAL | IE_GUI_BUTTON_ALIGN_LEFT | IE_GUI_BUTTON_ALIGN_BOTTOM
		Button.SetFlags (portraitFlags, OP_SET)

		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetPicture (Portrait["Sprite"], "NOPORTSM")
		ratio_str, color = GUICommon.SetupDamageInfo (pcID, Button, Window)

		# character - 1 == bam cycle, sometimes
		# only frames have all the glyphs
		# only bg2 and iwds have a proper blank glyph
		# so avoid using blanks except in bg2
		flag = blank = bytearray([33])
		if GameCheck.IsBG2():
			# only BG2 has icons for talk or store
			flag = blank = bytearray([238])
			talk = bytearray([154]) # dialog icon
			store = bytearray([155]) # shopping icon

			if pc == pcID and GemRB.GetStore() != None:
				flag = store
			# talk icon
			elif GemRB.GameGetSelectedPCSingle(1) == pcID:
				flag = talk

		if LUCommon.CanLevelUp (pcID):
			if GameCheck.IsBG2():
				flag = flag + blank + bytearray([255])
			else:
				flag = bytearray([255])
		else:
			if GameCheck.IsBG2():
				flag = flag + blank + blank
			else:
				flag = ""
			if GameCheck.IsIWD1() or GameCheck.IsIWD2():
				HPLabel = AddHPLabel (Window, Button, i) # missing if new pc joined since the window was opened
				HPLabel.SetText (ratio_str)
				HPLabel.SetColor (color)
			
		FlagLabel = AddStatusFlagLabel (Window, Button, i) # missing if new pc joined since the window was opened
		FlagLabel.SetText(flag)

		#add effects on the portrait
		effects = GemRB.GetPlayerStates (pcID)

		numCols = 4 if GameCheck.IsIWD2() else 3
		numEffects = len(effects)

		# calculate the partial row
		idx = numEffects % numCols
		states = bytearray(effects[0:idx])
		# now do any rows that are full
		for x in range(idx, numEffects):
			if (x - idx) % numCols == 0:
				states.append(ord('\n'))
			states.append(effects[x])

		Button.SetText(states)
	return

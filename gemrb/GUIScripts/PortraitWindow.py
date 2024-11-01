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
import GameCheck
import GUICommon
import GUICommonWindows
import LUCommon

from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *
from ie_stats import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

StatesFont = "STATES2"
if GameCheck.IsIWD1() or GameCheck.IsIWD2():
	StatesFont = "STATES"

ScreenHeight = GemRB.GetSystemVariable (SV_HEIGHT)

def SetupDamageInfo (Button):
	pc = Button.Value
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	state = GemRB.GetPlayerStat (pc, IE_STATE_ID)

	if hp_max < 1 or hp == "?":
		ratio = 0.0
	else:
		ratio = hp / float(hp_max)

	if hp < 1 or (state & STATE_DEAD):
		c = {'r' : 64, 'g' : 64, 'b' : 64, 'a' : 255}
		Button.SetOverlay (0, c, c)

	if ratio == 1.0:
		band = 0
		color = {'r' : 255, 'g' : 255, 'b' : 255}  # white
	elif ratio >= 0.75:
		band = 1
		color = {'r' : 0, 'g' : 255, 'b' : 0}  # green
	elif ratio >= 0.50:
		band = 2
		color = {'r' : 255, 'g' : 255, 'b' : 0}  # yellow
	elif ratio >= 0.25:
		band = 3
		color = {'r' : 255, 'g' : 128, 'b' : 0}  # orange
	else:
		band = 4
		color = {'r' : 255, 'g' : 0, 'b' : 0}  # red

	if GemRB.GetVar("Old Portrait Health") or not GameCheck.IsIWD2():
		# draw the blood overlay
		if hp >= 1 and not (state & STATE_DEAD):
			c1 = {'r' : 0x70, 'g' : 0, 'b' : 0, 'a' : 0xff}
			c2 = {'r' : 0xf7, 'g' : 0, 'b' : 0, 'a' : 0xff}
			Button.SetOverlay (ratio, c1, c2)
	else:
		# scale the hp bar under the portraits and recolor it
		# GUIHITPT has 5 frames with different severity colors
		# luckily their ids follow a nice pattern
		hpBar = Button.Window.GetControl (pc - 1 + 50)
		hpBar.SetBAM ("GUIHITPT", band, 0)
		hpBar.SetPictureClipping (ratio)
		hpBar.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)
		hpBar.SetState (IE_GUI_BUTTON_LOCKED)

	ratio_str = ""
	if hp != "?":
		ratio_str = "%d/%d" % (hp, hp_max)
	Button.SetTooltip (GemRB.GetPlayerName (pc, 1) + "\n" + ratio_str)

	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		HPLabel = Button.Window.GetControl(99 + pc)
		HPLabel.SetColor (color, TA_COLOR_NORMAL)
		HPLabel.SetText (ratio_str)

# returns buttons and a numerical index
# does nothing new in iwd2 due to layout
# in the rest, it will enable extra button generation for higher resolutions
# Mode determines arrangement direction, horizontal being for party reform and potentially save/load
def GetPortraitButtons (Window, ExtraSlots=0, Mode="vertical"):
	list = []

	oldSlotCount = 6 + ExtraSlots

	for i in range(min(oldSlotCount, MAX_PARTY_SIZE + ExtraSlots)): # the default chu/game limit or less
		btn = Window.GetControl(i)
		btn.SetHotKey(chr(ord('1') + i), 0, True)
		btn.SetVarAssoc("portrait", i + 1)
		list.append(btn)

	# nothing left to do
	PartySize = GemRB.GetPartySize ()
	if PartySize <= oldSlotCount:
		return list

	if GameCheck.IsIWD2():
		# set Mode = "horizontal" once we can create enough space
		GemRB.Log(LOG_ERROR, "GetPortraitButtons", "Parties larger than 6 are currently not supported in IWD2! Using 6 ...")
		return list

	# GUIWORLD doesn't have a separate portraits window, so we need to skip
	# all this magic when reforming an overflowing party
	if PartySize > MAX_PARTY_SIZE:
		return list

	# generate new buttons by copying from existing ones
	firstButton = list[0]
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

	for i in range(len(list), PartySize):
		if limitStep > limit:
			raise SystemExit("Not enough window space for so many party members (portraits), bailing out! %d vs width/height of %d/%d" %(limit, buttonWidth, buttonHeight))
		nextID = 1000 + i
		control = Window.GetControl (nextID)
		if control:
			list.append(control)
			continue
		if Mode ==  "horizontal":
			button = Window.CreateButton (nextID, xOffset+i*buttonWidth, yOffset, buttonWidth, buttonHeight)
		else:
			# vertical
			button = Window.CreateButton (nextID, xOffset, i*buttonHeight+yOffset+i*2*scale, buttonWidth, buttonHeight)

		button.SetVarAssoc("portrait", i + 1)
		button.SetSprites ("GUIRSPOR", 0, 0, 1, 0, 0)
		SetupButtonBorders (button)
		button.SetFont (StatesFont)
		button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)

		button.OnRightPress (GUICommonWindows.OpenInventoryWindowClick)
		button.OnPress (GUICommonWindows.PortraitButtonOnPress)
		button.OnShiftPress (GUICommonWindows.PortraitButtonOnShiftPress)
		button.SetAction (GUICommonWindows.PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		button.SetAction (GUICommonWindows.ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		button.SetAction (GUICommonWindows.ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)

		list.append(button)
		limit -= limitStep

	# move the buttons back up, to combine the freed space
	if scale:
		for i in range(PartySize - 1):
			button = list[i]
			button.SetSize (buttonWidth, buttonHeight)
			if i == 0:
				continue # don't move the first portrait
			rect = button.GetFrame ()
			x = rect["x"]
			y = rect["y"]
			button.SetPos (x, y-portraitGap*i)

	return list

def AddStatusFlagLabel (Button, i):
	# label for status flags (dialog, store, level up)
	align = IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_RIGHT | IE_FONT_SINGLE_LINE
	if GameCheck.IsBG2 ():
		align = IE_FONT_ALIGN_TOP | IE_FONT_ALIGN_CENTER | IE_FONT_SINGLE_LINE
	label = Button.CreateLabel (199 + i, StatesFont, "", align) #level up icon is on the right
	label.SetFrame (Button.GetInsetFrame (4))
	return label

# overlay a label, so we can display the hp with the correct font. Regular button label
#   is used by effect icons
def AddHPLabel (Button, i):
	iwd1 = GameCheck.IsIWD1 ()
	frame = Button.GetInsetFrame (3 + iwd1 * 3, 2, 2, 1 + iwd1);
	label = Button.CreateTextArea (99 + i, frame["x"], frame["y"], frame["w"], 10, "NUMFONT")
	label.SetColor (ColorBlackish, TA_COLOR_BACKGROUND)
	label.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	return label

def SetupButtonBorders (Button):
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

def MinimizePortraits(): #bg2
	GemRB.GameSetScreenFlags(GS_PORTRAITPANE, OP_OR)

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
				Button.OnPress (GUICommonWindows.RestPress)
			else:
				Button.OnPress (MinimizePortraits)
		else:
			if GameCheck.HasHOW():
				# Rest (how)
				pos = Window.GetFrame()["h"] - 37
				Button = Window.CreateButton (8, 6, pos, 55, 37)
				Button.SetSprites ("GUIRSBUT", 0,0,1,0,0)
				Button.SetTooltip (11942)
				Button.OnPress (GUICommonWindows.RestPress)

				pos = pos - 37
				Window.CreateButton (6, 6, pos, 27, 36)

		# AI
		Button = Window.GetControl (6)
		#fixing a gui bug, and while we are at it, hacking it to be easier
		Button.SetSprites ("GUIBTACT", 0, 46, 47, 48, 49)
		GUICommonWindows.InitOptionButton(Window, 'Toggle_AI', GUICommonWindows.AIPress)
		GUICommonWindows.AIPress(0) #this initialises the state and tooltip

		#Select All
		if GameCheck.HasHOW():
			Button = Window.CreateButton (7, 33, pos, 27, 36)
			Button.SetSprites ("GUIBTACT", 0, 50, 51, 50, 51)
		else:
			Button = Window.GetControl (7)
		Button.SetTooltip (10485)
		Button.OnPress (GUICommon.SelectAllOnPress)
	elif not GameCheck.IsIWD2(): # Rest
		Button = Window.GetControl (6)
		if Button:
			Button.SetTooltip (11942)
			Button.OnPress (GUICommonWindows.RestPress)

	PortraitButtons = GetPortraitButtons (Window)
	for Button in PortraitButtons:
		pcID = Button.Value

		Button.SetVarAssoc("portrait", pcID)
		Button.SetHotKey(chr(ord('0') + pcID), 0, True)
		Button.SetFont (StatesFont)
		AddStatusFlagLabel (Button, pcID)

		if needcontrols or GameCheck.IsIWD2():
			Button.OnRightPress (GUICommonWindows.OpenInventoryWindowClick)
		else:
			Button.OnRightPress (GUICommonWindows.PortraitButtonOnPress)

		Button.OnPress (GUICommonWindows.PortraitButtonOnPress)
		Button.OnShiftPress (GUICommonWindows.PortraitButtonOnShiftPress)
		Button.SetAction (GUICommonWindows.PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		Button.SetAction (GUICommonWindows.ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		Button.SetAction (GUICommonWindows.ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)
		Button.SetAction(lambda btn: GemRB.GameControlLocateActor(btn.Value), IE_ACT_MOUSE_ENTER);
		Button.SetAction(lambda: GemRB.GameControlLocateActor(-1), IE_ACT_MOUSE_LEAVE);

		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_HORIZONTAL | IE_GUI_BUTTON_ALIGN_LEFT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_SET)
		Button.SetState (IE_GUI_BUTTON_LOCKED)

		SetupButtonBorders (Button)
		if GameCheck.IsIWD1() or GameCheck.IsIWD2():
			AddHPLabel (Button, pcID)

	UpdatePortraitWindow()
	return Window

def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = GemRB.GetView("PORTWIN")
	if not Window:
		return

	pc = GemRB.GameGetSelectedPCSingle ()

	def SetIcons(Button):
		pcID = Button.Value
		# character - 1 == bam cycle
		talk = store = flag = blank = bytearray([32])
		if GameCheck.IsBG2():
			flag = blank = bytearray([238])
			# as far as I can tell only BG2 has icons for talk or store
			flag = bytearray([238])

			if pcID == GemRB.GetVar("DLG_SPEAKER"):
				flag = bytearray([154]) # dialog icon
			elif pcID == GemRB.GetVar("BARTER_PC") and not GemRB.GetView("WIN_INV"): # don't show in bags
				flag = bytearray([155]) # shopping icon

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

		FlagLabel = Window.GetControl (199 + pcID)
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

	def EnablePortrait(Button):
		Button.SetVisible(True)

		pcID = Button.Value
		Portrait = GemRB.GetPlayerPortrait (pcID, 1)

		pic = Portrait["Sprite"] if Portrait else ""
		Button.SetPicture (pic, "NOPORTSM")
		SetIcons(Button)
		SetupDamageInfo(Button)

		if GUICommonWindows.SelectionChangeHandler:
			Button.EnableBorder(FRAME_PC_SELECTED, pc == pcID)
		else:
			Button.EnableBorder(FRAME_PC_SELECTED, GemRB.GameIsPCSelected(pcID))

	for Button in GetPortraitButtons(Window):
		pcID = Button.Value
		if pcID > GemRB.GetPartySize():
			Button.SetVisible(False)
		elif pc and pcID != pc and GemRB.GetView("WIN_STORE") and GemRB.GetView("WIN_INV"):
			# opened a bag in inventory
			Button.SetVisible(False)
		elif GemRB.GetPlayerStat(pcID, IE_STATE_ID) & STATE_DEAD and GemRB.GetView("WIN_STORE") and not GemRB.GetView("WINHEAL"):
			# dead pcs are hidden in all stores but temples
			Button.SetVisible(False)
		else:
			EnablePortrait(Button)

	return

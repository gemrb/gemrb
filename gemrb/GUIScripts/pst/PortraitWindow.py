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

import GemRB
import GUICommonWindows
from GUIDefines import *
from ie_stats import *

def GetPortraitButtonPairs(Window):
	return {i : Window.GetControl(i) for i in range(6)}

def PortraitButtonHPOnPress (btn):
	id = btn.ControlID - 6
	hbs = GemRB.GetVar('Health Bar Settings')
	if hbs & (1 << id):
		op = OP_NAND
	else:
		op = OP_OR

	btn.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)
	GemRB.SetVar('Health Bar Settings', hbs ^ (1 << id))
	return

def SetupButtonBorders (Button):
	yellow = {'r' : 255, 'g' : 255, 'b' : 0, 'a' : 255}
	green = {'r' : 0, 'g' : 255, 'b' : 0, 'a' : 255}
	Button.SetBorder (0, green, 0, 0, Button.GetInsetFrame (1, 1, 2, 2))
	Button.SetBorder (1, yellow, 0, 0, Button.GetInsetFrame (3, 3, 4, 4))

def OpenPortraitWindow (pos=WINDOW_RIGHT|WINDOW_VCENTER):
	Window = GemRB.LoadWindow (1, "GUIWORLD", pos)
	Window.AddAlias("PORTWIN")
	Window.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	# Rest
	Button = Window.GetControl (6)
	Button.SetTooltip (11942)
	Button.OnPress (GUICommonWindows.RestPress)

	PortraitButtons = GetPortraitButtonPairs (Window)
	for i, Button in PortraitButtons.items():
		pcID = i + 1

		Button.OnRightPress (GUICommonWindows.OpenInventoryWindowClick)
		Button.OnPress (GUICommonWindows.PortraitButtonOnPress)
		Button.SetAction (GUICommonWindows.PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		Button.SetAction (GUICommonWindows.ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		Button.SetAction (GUICommonWindows.ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)
		Button.SetBAM ("PPPANN", 0, 0, -1) # NOTE: just a dummy, won't be visible
		Button.SetVarAssoc("portrait", pcID) # neeeded for drag/drop
		Button.SetHotKey(chr(ord('1') + i), 0, True)
		SetupButtonBorders (Button)
			
		ButtonHP = Window.GetControl (6 + i)
		ButtonHP.SetFlags(IE_GUI_BUTTON_PICTURE, OP_SET)
		ButtonHP.SetBAM('FILLBAR', 0, 0, -1)
		ButtonHP.OnPress(PortraitButtonHPOnPress)

	UpdatePortraitWindow ()
	return Window
	
def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = GemRB.GetView("PORTWIN")

	PortraitButtons = GetPortraitButtonPairs (Window)
	for i, Button in PortraitButtons.items():
		pcID = i + 1

		if pcID <= GemRB.GetPartySize():
			Button.SetAction(lambda btn, pc=pcID: GemRB.GameControlLocateActor(pc), IE_ACT_MOUSE_ENTER)
			Button.SetAction(lambda: GemRB.GameControlLocateActor(-1), IE_ACT_MOUSE_LEAVE)
		UpdatePortraitButton(Button)

	return

def UpdatePortraitButton(Button):
	"""Selects the correct portrait cycle depending on character state"""
	# note: there are actually two portraits per chr, eg PPPANN (static), WMPANN (animated)
	pcID = Button.ControlID + 1
	portrait = GemRB.GetPlayerPortrait (pcID, 0)
	if not portrait:
		Button.SetVisible (False)
		return

	Button.SetVisible (True)
	
	state = GemRB.GetPlayerStat (pcID, IE_STATE_ID)
	hp = GemRB.GetPlayerStat (pcID, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pcID, IE_MAXHITPOINTS)

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
	elif hp < hp_max / 2:
		cycle = 4
	else:
		cycle = 0

	if cycle < 6:
		Button.SetFlags (IE_GUI_BUTTON_PLAYRANDOM, OP_OR)

	Button.SetAnimation(portrait["ResRef"], cycle)

	if hp_max < 1 or hp == "?":
		ratio = 0.0
	else:
		ratio = hp / float(hp_max)
		if ratio > 1.0: ratio = 1.0

	r = int (255 * (1.0 - ratio))
	g = int (255 * ratio)

	ButtonHP = Button.Window.GetControl(6 + Button.ControlID)
	ButtonHP.SetText ("{} / {}".format(hp, hp_max))
	ButtonHP.SetColor ({'r' : r, 'g' : g, 'b' : 0})
	ButtonHP.SetPictureClipping (ratio)

	if GemRB.GetVar('Health Bar Settings') & (1 << Button.ControlID):
		op = OP_OR
	else:
		op = OP_NAND
	ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_NO_TEXT, op)

	return

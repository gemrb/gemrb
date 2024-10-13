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

import random

import GemRB
import GUICommonWindows
from GUICommon import BindControlCallbackParams
from GUIDefines import *
from ie_stats import *

def GetPortraitButtons(Window, *args):
	list = []
	for i in range(6):
		btn = Window.GetControl(i)
		btn.SetVarAssoc("portrait", i + 1)
		list.append(btn)
	return list

def PortraitButtonHPOnPress (btn):
	id = btn.ControlID - 6
	hbs = GemRB.GetVar('Health Bar Settings')
	GemRB.SetVar('Health Bar Settings', hbs ^ (1 << id))
	UpdatePortraitButton(btn.Window.GetControl(id))
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

	PortraitButtons = GetPortraitButtons (Window)
	for Button in PortraitButtons:
		pcID = Button.Value

		Button.OnRightPress (GUICommonWindows.OpenInventoryWindowClick)
		Button.OnPress (GUICommonWindows.PortraitButtonOnPress)
		Button.SetAction (GUICommonWindows.PortraitButtonOnShiftPress, IE_ACT_MOUSE_PRESS, GEM_MB_ACTION, GEM_MOD_CTRL, 1)
		Button.SetAction (GUICommonWindows.ButtonDragSourceHandler, IE_ACT_DRAG_DROP_SRC)
		Button.SetAction (GUICommonWindows.ButtonDragDestHandler, IE_ACT_DRAG_DROP_DST)
		Button.SetAction(BindControlCallbackParams(GemRB.GameControlLocateActor, pcID), IE_ACT_MOUSE_ENTER)
		Button.SetAction(lambda: GemRB.GameControlLocateActor(-1), IE_ACT_MOUSE_LEAVE)
		Button.SetBAM ("PPPANN", 0, 0, -1) # NOTE: just a dummy, won't be visible
		Button.SetHotKey(chr(ord('0') + pcID), 0, True)
		Button.OnAnimEnd (UpdateButtonAnimation)
		SetupButtonBorders (Button)

		ButtonHP = Window.GetControl (5 + pcID)
		ButtonHP.SetFlags(IE_GUI_BUTTON_PICTURE, OP_SET)
		ButtonHP.SetBAM('FILLBAR', 0, 0, -1)
		ButtonHP.OnPress(PortraitButtonHPOnPress)

	UpdatePortraitWindow()
	return Window
	
def UpdatePortraitWindow ():
	"""Updates all of the portraits."""

	Window = GemRB.GetView("PORTWIN")

	pc = GemRB.GameGetSelectedPCSingle ()

	PortraitButtons = GetPortraitButtons (Window)
	for Button in PortraitButtons:
		pcID = Button.Value

		UpdatePortraitButton(Button)
		if GUICommonWindows.SelectionChangeHandler:
			Button.EnableBorder(0, pc == pcID)
		else:
			Button.EnableBorder(0, GemRB.GameIsPCSelected(pcID))

	return

def UpdateButtonAnimation(Button):
	"""Selects the correct portrait cycle depending on character state"""
	# note: there are actually two portraits per chr, eg PPPANN (static), WMPANN (animated)

	pcID = Button.ControlID + 1

	state = GemRB.GetPlayerStat (pcID, IE_STATE_ID)
	hp = GemRB.GetPlayerStat (pcID, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pcID, IE_MAXHITPOINTS)

	if state & STATE_DEAD:
		cycle = 9
	elif state & STATE_HELPLESS:
		cycle = 8
	elif state & STATE_PETRIFIED:
		cycle = 7
	elif state & STATE_PANIC: # frozen?
		cycle = 6
	elif state & STATE_POISONED:
		cycle = 2 # and 3
	elif hp < hp_max / 2:
		cycle = 4 # and 5
	else:
		cycle = 0 # and 1

	portrait = GemRB.GetPlayerPortrait (pcID, 0)
	pic = portrait["ResRef"]

	if cycle < 6: # a "random" animation, other cycles are not animated
		def phase2(btn):
			switch = random.randint(0, 29) == 0
			btn.SetAnimation(pic, cycle + int(switch), A_ANI_PLAYONCE)
			btn.OnAnimEnd (UpdateButtonAnimation)
		# note: the granularity of time should be
		# one of twenty values from [500, 10000]
		# but not the full range.
		duration = 500 + 500 * random.randint(0, 19);
		Button.SetAnimation([GemRB.GetSprite(pic, -1, cycle, 0)], duration, A_ANI_PLAYONCE)
		Button.OnAnimEnd (phase2)
	else:
		Button.SetAnimation(pic, cycle, A_ANI_PLAYONCE)
		Button.OnAnimEnd (None)
	return

def UpdatePortraitButton(Button):
	pcID = Button.ControlID + 1
	portrait = GemRB.GetPlayerPortrait (pcID, 0)
	if not portrait:
		Button.SetVisible(False)
		Button.SetAnimation(None)
		return

	Button.SetVisible (True)

	hp = GemRB.GetPlayerStat (pcID, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pcID, IE_MAXHITPOINTS)

	if hp_max < 1 or hp == "?":
		ratio = 0.0
	else:
		ratio = hp / float(hp_max)
		if ratio > 1.0: ratio = 1.0

	r = int (255 * (1.0 - ratio))
	g = int (255 * ratio)

	ButtonHP = Button.Window.GetControl(6 + Button.ControlID)

	ButtonHP.SetColor ({'r' : r, 'g' : g, 'b' : 0})
	ButtonHP.SetPictureClipping (ratio)

	if GemRB.GetVar('Health Bar Settings') & (1 << Button.ControlID):
		ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE, OP_OR)
		ButtonHP.SetText ("")
	else:
		ButtonHP.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		ButtonHP.SetText ("{} / {}".format(hp, hp_max))

	GemRB.GetVar ("{}_ANIM".format(Button.ControlID))

	UpdateButtonAnimation(Button)
	return

# GemRB - Infinity Engine Emulator
# Copyright (C) 2022 The GemRB Project
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

# PartyReform.py - scripts to control party reformation windows from the GUIWORLD winpack
#################################################################

import GemRB

import CommonWindow
import GameCheck
import GUICommon
import PortraitWindow
from GUIDefines import *
from ie_stats import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

RemovablePCs = []

def UpdateReformWindow (Window, select):
	needToDrop = GemRB.GetPartySize () - GameCheck.MAX_PARTY_SIZE
	if needToDrop < 0:
		needToDrop = 0

	# excess player number
	Label = Window.GetControl (0x1000000f)
	Label.SetText (str(needToDrop))

	# done
	Button = Window.GetControl (8)
	if needToDrop:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	# remove
	Button = Window.GetControl (15)
	Button.SetText (42514 if GameCheck.IsPST () else 17507)
	Button.OnPress (lambda: RemovePlayer(select))
	if select:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	PortraitButtons = PortraitWindow.GetPortraitButtonPairs (Window, 1, "horizontal")
	for i in PortraitButtons:
		Button = PortraitButtons[i]
		if i + 1 not in RemovablePCs:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	for i in RemovablePCs:
		idx = RemovablePCs.index(i)
		if idx not in PortraitButtons:
			continue # for saved games with higher party count than the current setup supports
		Button = PortraitButtons[idx]
		Button.EnableBorder (FRAME_PC_SELECTED, select == i)
		portrait = GemRB.GetPlayerPortrait (i, 1)
		pic = portrait["Sprite"]
		if not pic and not portrait["ResRef"]:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		Button.SetPicture (pic, "NOPORTSM")
	PortraitWindow.UpdatePortraitWindow()
	return

def RemovePlayer (select):
	hideFlag = CommonWindow.IsGameGUIHidden ()

	winID = 25
	if GameCheck.IsHOW ():
		winID = 0 # at least in guiw08, this is the correct window
	elif GameCheck.IsPST ():
		winID = 26 # so we get two buttons
	Window = GemRB.LoadWindow (winID, GUICommon.GetWindowPack (), WINDOW_BOTTOM | WINDOW_HCENTER)

	# are you sure
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (28071 if GameCheck.IsPST () else 17518)

	# confirm
	def RemovePlayerConfirm ():
		if GameCheck.IsBG2 ():
			GemRB.LeaveParty (select, 2)
		elif GameCheck.IsBG1 ():
			GemRB.LeaveParty (select, 1)
		else:
			GemRB.LeaveParty (select)

		Window.Close ()
		GemRB.GetView ("WIN_REFORM", 0).Close ()
		return

	Button = Window.GetControl (1)
	Button.SetText (42514 if GameCheck.IsPST () else 17507)
	Button.OnPress (RemovePlayerConfirm)
	Button.MakeDefault()

	#cancel
	Button = Window.GetControl (2)
	Button.SetText (4196 if GameCheck.IsPST () else 13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	CommonWindow.SetGameGUIHidden (hideFlag)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenReformPartyWindow ():
	global RemovablePCs

	hideFlag = CommonWindow.IsGameGUIHidden()

	Window = GemRB.LoadWindow (24, GUICommon.GetWindowPack(), WINDOW_HCENTER | WINDOW_BOTTOM)
	Window.AddAlias ("WIN_REFORM", 0)

	# skip exportable party members (usually only the protagonist)
	RemovablePCs = []
	for i in range (1, GemRB.GetPartySize() + 1):
		if not GemRB.GetPlayerStat (i, IE_MC_FLAGS) & MC_EXPORTABLE:
			RemovablePCs.append (i)

	# PC portraits
	PortraitButtons = PortraitWindow.GetPortraitButtonPairs (Window, 1, "horizontal")
	for j in PortraitButtons:
		Button = PortraitButtons[j]
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON | IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, OP_SET)
		color = {'r' : 0, 'g' : 255, 'b' : 0, 'a' : 255}
		Button.SetBorder (FRAME_PC_SELECTED, color, 0, 0, Button.GetInsetFrame (1, 1, 2, 2))
		if j < len(RemovablePCs):
			Button.SetValue (RemovablePCs[j])
		else:
			Button.SetValue (None)

		Button.OnPress (lambda btn: UpdateReformWindow (Window, btn.Value))

	# Done
	Button = Window.GetControl (8)
	Button.SetText (1403 if GameCheck.IsPST () else 11973)
	Button.OnPress (Window.Close)

	# if nobody can be removed, just close the window
	if not RemovablePCs:
		Window.Close ()
		CommonWindow.SetGameGUIHidden (hideFlag)
		return

	UpdateReformWindow (Window, None)
	CommonWindow.SetGameGUIHidden (hideFlag)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

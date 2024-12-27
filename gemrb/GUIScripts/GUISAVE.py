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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#


# GUISAVE.py - Save game screen from GUISAVE winpack

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
import LoadScreen
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *

SaveWindow = None
ConfirmWindow = None
SaveButton = 0
Games = ()
ScrollBar = 0
# this lookup table is used only by bg2
str_chapter = (48007, 48006, 16205, 16206, 16207, 16208, 16209, 71020, 71021, 71022)
num_rows = 4
ctrl_offset = (26, 30, 40, 0x10000008, 0x10000010, 25, 34, 3, 0x10000004, 40, 7, 8, 2)
sav_version = 0
strs = { 'cancel' : 13727, 'save' : 15588, 'delete' : 13957, 'empty' : 15304, 'overwrite' : 15306, 'yousure' : 15305 }

def OpenSaveWindow ():
	global SaveWindow, Games, ScrollBar
	global num_rows, ctrl_offset, sav_version, strs

	if GameCheck.IsIWD2 ():
		num_rows = 5
		ctrl_offset = (55, 60, 25, 0x10000005, 0x1000000a, 23, 22, 3, 0x10000004, 40, 7, 8, 2)
		sav_version = 22
	else:
		if GameCheck.IsPST ():
			ctrl_offset = (14, 18, 22, 0x10000004, 0x10000008, 13, 46, 1, 0x10000002, 6, 4, 5, 3)
			strs = { 'cancel' : 4196, 'save' : 28645, 'delete' : 28640, 'empty' : 28647, 'overwrite' : 28644, 'yousure' : 28639 }

	SaveWindow = Window = GemRB.LoadWindow (0, "GUISAVE")

	# Cancel button
	CancelButton = Window.GetControl (ctrl_offset[6])
	CancelButton.SetText (strs['cancel'])
	CancelButton.OnPress (CloseSaveWindow)
	CancelButton.MakeEscape ()

	for i in range(num_rows):
		Button = Window.GetControl (ctrl_offset[0]+i)
		Button.SetText (strs['save'])
		Button.OnPress (OpenConfirmWindow)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetValue (i)

		Button = Window.GetControl (ctrl_offset[1]+i)
		Button.SetText (strs['delete'])
		Button.OnPress (DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetValue (i)

		# area previews
		Button = Window.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)

		# PC portraits
		for j in range(min(6, MAX_PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, MAX_PARTY_SIZE) + j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)
			if GameCheck.IsIWD2 ():
				Button.SetSize (21, 21)

	ScrollBar = Window.GetControl (ctrl_offset[5])
	ScrollBar.OnChange (ScrollBarPress)
	GUICommon.SetSaveDir ()
	Games = GemRB.GetSaveGames ()
	TopIndex = max (0, len(Games) - num_rows + 1) #one more for the 'new game'
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, TopIndex)
	Window.SetEventProxy (ScrollBar)
	ScrollBarPress ()
	Window.Focus ()
	return

def ScrollBarPress ():
	Window = SaveWindow

	# draw load game portraits
	Pos = GemRB.GetVar ("TopIndex") or 0
	for i in range(num_rows):
		ActPos = Pos + i

		Button1 = Window.GetControl (ctrl_offset[0]+i)
		Button2 = Window.GetControl (ctrl_offset[1]+i)
		if ActPos <= len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)

		if ActPos < len(Games):
			Slotname = Games[ActPos].GetName ()
			Slottime = Games[ActPos].GetGameDate ()
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
		elif ActPos == len(Games):
			Slotname = strs['empty']
			Slottime = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
		else:
			Slotname = ""
			Slottime = ""
			Button2.SetState (IE_GUI_BUTTON_DISABLED)

		Label = Window.GetControl (ctrl_offset[3]+i)
		Label.SetText (Slotname)

		Label = Window.GetControl (ctrl_offset[4]+i)
		Label.SetText (Slottime)

		Button = Window.GetControl (1+i)
		if ActPos < len(Games):
			Button.SetPicture (Games[ActPos].GetPreview())
		else:
			Button.SetPicture (None)

		for j in range(min(6, MAX_PARTY_SIZE)):
			Button = Window.GetControl (ctrl_offset[2] + i*min(6, MAX_PARTY_SIZE) + j)
			if ActPos < len(Games):
				Button.SetPicture (Games[ActPos].GetPortrait(j))
			else:
				Button.SetPicture (None)
	return

def QuickSavePressed ():
	Slot = 1
	GUICommon.SetSaveDir ()

	if GameCheck.IsTOB():
		Slot = 4

	GemRB.SaveGame(Slot)
	return

def CloseConfirmWindow():
	global ConfirmWindow

	if ConfirmWindow:
		ConfirmWindow.Close ()
		ConfirmWindow = None

	SaveWindow.Focus ()
	return

# User entered save name and pressed save/overwrite.
# Display progress bar screen and save the game, close the save windows
def ConfirmedSaveGame (Pos):
	global ConfirmWindow

	Label = ConfirmWindow.GetControl (ctrl_offset[7])
	Slotname = Label.QueryText ()
	Slotname = Slotname.replace ("/", "|") # can't have path separators in the name

	# Empty save name. We can get here if user presses Enter key
	if Slotname == "":
		return

	# We have to close floating window first
	CloseConfirmWindow ()
	#FIXME: make this work
	#LoadScreen.StartLoadScreen (LoadScreen.LS_TYPE_SAVING)

	CloseSaveWindow ()
	GUICommonWindows.CloseTopWindow ()

	if Pos < len(Games):
		GemRB.SaveGame (Games[Pos], Slotname, sav_version)
	else:
		GemRB.SaveGame (None, Slotname, sav_version)
	return

def OpenConfirmWindow (btn):
	global ConfirmWindow, SaveButton

	Pos = GemRB.GetVar ("TopIndex") + btn.Value
	ConfirmWindow = GemRB.LoadWindow (1, "GUISAVE")

	AreaPreview = ConfirmWindow.GetControl (0)
	if Pos < len(Games):
		Slotname = Games[Pos].GetName()
		save_strref = strs['overwrite']

		if GameCheck.IsBG2OrEE ():
			Chapter = GemRB.GetGameVar ("CHAPTER") & 0x7fffffff
			GameDate = GemRB.GetString (str_chapter[Chapter-1]) + " " + Games[Pos].GetGameDate()
		else:
			GameDate = Games[Pos].GetGameDate ()

		if AreaPreview:
			AreaPreview.SetPicture (Games[Pos].GetPreview())
	else:
		Slotname = ""
		GameDate = ""
		save_strref = strs['save']
		if AreaPreview:
			AreaPreview.SetPicture (None)

	NameField = ConfirmWindow.GetControl (ctrl_offset[7])
	NameField.SetText (Slotname)
	NameField.OnChange (EditChange)

	Label = ConfirmWindow.GetControl (ctrl_offset[8])
	Label.SetText (GameDate)

	# PC portraits
	for j in range(min(6, MAX_PARTY_SIZE)):
		Portrait = ConfirmWindow.GetControl (ctrl_offset[9] + j)
		if not Portrait:
			continue
		if Pos < len(Games):
			Portrait.SetPicture (Games[Pos].GetPortrait(j))
		else:
			Portrait.SetPicture (None)

	# Save/Overwrite
	SaveButton = ConfirmWindow.GetControl (ctrl_offset[10])
	SaveButton.SetText (save_strref)
	SaveButton.OnPress (lambda: ConfirmedSaveGame (Pos))
	SaveButton.MakeDefault ()

	if Slotname == "":
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)

	# Cancel
	CancelButton = ConfirmWindow.GetControl (ctrl_offset[11])
	CancelButton.SetText (strs['cancel'])
	CancelButton.OnPress (CloseConfirmWindow)
	CancelButton.MakeEscape ()

	ConfirmWindow.Focus ()
	ConfirmWindow.ShowModal (MODAL_SHADOW_NONE)
	NameField.Focus () # ShowModal will happily reset this..
	return

# Disable Save/Overwrite button if the save slotname is empty,
# else enable it
def EditChange (NameField):
	Name = NameField.QueryText ()
	if len(Name) == 0:
		SaveButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		SaveButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DeleteGameConfirm (delIndex):
	global Games, num_rows

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex + delIndex
	GemRB.DeleteSaveGame (Games[Pos])
	del Games[Pos]
	if TopIndex > 0:
		TopIndex = TopIndex - 1
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, max (0, len(Games) - num_rows + 1))
	ScrollBarPress ()

	CloseConfirmWindow ()
	return

def DeleteGamePress (btn):
	global ConfirmWindow

	ConfirmWindow = GemRB.LoadWindow (ctrl_offset[12], "GUISAVE")
	ConfirmWindow.SetFlags (WF_ALPHA_CHANNEL, OP_OR)
	ConfirmWindow.ShowModal (MODAL_SHADOW_GRAY)

	Text = ConfirmWindow.GetControl (0)
	Text.SetText (strs['yousure'])

	DeleteButton = ConfirmWindow.GetControl (1)
	DeleteButton.SetText (strs['delete'])
	DeleteButton.OnPress (lambda: DeleteGameConfirm (btn.Value))
	DeleteButton.MakeDefault ()

	CancelButton = ConfirmWindow.GetControl (2)
	CancelButton.SetText (strs['cancel'])
	CancelButton.OnPress (CloseConfirmWindow)
	CancelButton.MakeEscape()

	ConfirmWindow.Focus()
	return

# Exit either back to game or to the Start window
def CloseSaveWindow ():
	global SaveWindow

	if SaveWindow:
		SaveWindow.Close ()
		SaveWindow = None

	if GemRB.GetVar ("QuitAfterSave"):
		GemRB.QuitGame ()
		GemRB.SetNextScript ("Start")

	return

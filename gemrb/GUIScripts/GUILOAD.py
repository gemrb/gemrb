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


# GUILOAD.py - Load window

###################################################

import GemRB
import LoadScreen
import GameCheck
import GUICommon

from GameCheck import MAX_PARTY_SIZE

LoadWindow = 0
TextAreaControl = 0
Games = ()
ScrollBar = 0

def OnLoad ():
	global LoadWindow, TextAreaControl, Games, ScrollBar

	if GameCheck.IsIWD1():
		GemRB.SetVar ("PlayMode", 0) # old code, not sure if needed
		
	GUICommon.SetSaveDir ()
	LoadWindow = GemRB.LoadWindow (0, "GUILOAD")

	CancelButton=LoadWindow.GetControl (34)
	CancelButton.SetText (13727)
	CancelButton.OnPress (LoadWindow.Close)
	CancelButton.MakeEscape()

	for i in range (4):
		Button = LoadWindow.GetControl (26+i)
		Button.SetText (15590)
		Button.OnPress (LoadGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetValue (i)

		Button = LoadWindow.GetControl (30+i)
		Button.SetText (13957)
		Button.OnPress (DeleteGamePress)
		Button.SetState (IE_GUI_BUTTON_DISABLED)
		Button.SetValue (i)

		#area previews
		Button = LoadWindow.GetControl (1+i)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

		#PC portraits
		for j in range (min(6, MAX_PARTY_SIZE)):
			Button = LoadWindow.GetControl (40 + i*min(6, MAX_PARTY_SIZE) + j)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)

	ScrollBar = LoadWindow.GetControl (25)
	ScrollBar.OnChange (ScrollBarUpdated)
	Games = GemRB.GetSaveGames ()
	TopIndex = max (0, len(Games) - 4)
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, TopIndex)
	LoadWindow.SetEventProxy (ScrollBar)
	LoadWindow.Focus ()
	return

def ScrollBarUpdated (sb):
	#draw load game portraits
	for i in range (4):
		ActPos = sb.Value + i

		Button1 = LoadWindow.GetControl (26+i)
		Button2 = LoadWindow.GetControl (30+i)
		PreviewButton = LoadWindow.GetControl (1+i)
		SlotName = ""
		SlotDate = ""
		if ActPos < len(Games):
			Button1.SetState (IE_GUI_BUTTON_ENABLED)
			Button2.SetState (IE_GUI_BUTTON_ENABLED)
			SlotName = Games[ActPos].GetName ()
			SlotDate = Games[ActPos].GetGameDate ()
			if GameCheck.IsBG1 ():
				ChapterTitle = GemRB.GetString (16202 + int(GemRB.GetToken ("CHAPTER0")))
				SlotDate = ChapterTitle + ". " + SlotDate
			PreviewButton.SetPicture (Games[ActPos].GetPreview())
		else:
			Button1.SetState (IE_GUI_BUTTON_DISABLED)
			Button2.SetState (IE_GUI_BUTTON_DISABLED)
			PreviewButton.SetPicture (None)

		Label = LoadWindow.GetControl (0x10000008+i)
		Label.SetText (SlotName)

		Label = LoadWindow.GetControl (0x10000010+i)
		Label.SetText (SlotDate)

		for j in range (min(6, MAX_PARTY_SIZE)):
			Button = LoadWindow.GetControl (40 + i*min(6, MAX_PARTY_SIZE) + j)
			if ActPos < len(Games):
				Button.SetPicture (Games[ActPos].GetPortrait (j))
			else:
				Button.SetPicture (None)
	return
	
def OpenLoadMsgWindow (btn):
	from GUIOPTControls import STR_OPT_CANCEL
	LoadMsgWindow = Window = GemRB.LoadWindow (4, "GUIOPT")
	Window.SetFlags (WF_BORDERLESS, OP_OR)
	
	def AbandonGame():
		GemRB.QuitGame()
		GemRB.SetTimer(lambda: LoadGamePress(btn), 0, 0)

	# Load
	Button = Window.GetControl (0)
	Button.SetText (15590)
	Button.OnPress (AbandonGame)
	Button.MakeDefault ()

	# Cancel
	Button = Window.GetControl (1)
	Button.SetText (STR_OPT_CANCEL)
	Button.OnPress (LoadMsgWindow.Close)
	Button.MakeEscape ()

	# Loading a game will destroy ...
	Text = Window.GetControl (3)
	Text.SetText (19531)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def LoadGamePress (btn):
	if GemRB.GetView ("GC"): # FIXME: is this the best way to know if we are ingame?
		OpenLoadMsgWindow (btn)
		return
	
	Pos = GemRB.GetVar ("TopIndex") + btn.Value
	#loads savegame
	GemRB.LoadGame (Games[Pos])

	#enters game
	if GameCheck.IsIWD1 ():
		GemRB.SetNextScript ("PartyFormation")
	else:
		LoadScreen.StartLoadScreen ()
		# it will close windows, including the loadscreen
		GemRB.EnterGame ()
		GemRB.SoftEndPL ()
	return

def GetQuickLoadSlot():
	global Games

	Games = GemRB.GetSaveGames()
	QuickLoadSlot = None
	for Game in Games:
		Slotname = Game.GetSaveID ()
		# quick save is 1
		if Slotname == 1:
			QuickLoadSlot = Game
			break
	return QuickLoadSlot

def QuickLoadPressed():
	QuickLoadSlot = GetQuickLoadSlot ()
	if QuickLoadSlot != None:
		LoadScreen.StartLoadScreen ()
		GemRB.LoadGame (QuickLoadSlot)
		GemRB.EnterGame ()
	return

def DeleteGameConfirm (btn):
	global Games

	TopIndex = GemRB.GetVar ("TopIndex")
	Pos = TopIndex + btn.Value
	GemRB.DeleteSaveGame(Games[Pos])
	del Games[Pos]

	if TopIndex > 0:
		TopIndex = TopIndex - 1
	ScrollBar.SetVarAssoc ("TopIndex", TopIndex, 0, max(0, len(Games) - 4))

	if ConfirmWindow:
		ConfirmWindow.Close ()
	LoadWindow.Focus()
	return

def DeleteGameCancel ():
	if ConfirmWindow:
		ConfirmWindow.Close ()
	LoadWindow.Focus()
	return

def DeleteGamePress (btn):
	global ConfirmWindow

	ConfirmWindow = GemRB.LoadWindow (1)
	ConfirmWindow.SetFlags (WF_ALPHA_CHANNEL, OP_OR)
	Text=ConfirmWindow.GetControl (0)
	Text.SetText (15305)
	DeleteButton = ConfirmWindow.GetControl (1)
	DeleteButton.SetText (13957)
	DeleteButton.OnPress (DeleteGameConfirm)
	DeleteButton.SetValue (btn.Value)
	CancelButton = ConfirmWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.OnPress (DeleteGameCancel)
	CancelButton.MakeEscape ()

	ConfirmWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

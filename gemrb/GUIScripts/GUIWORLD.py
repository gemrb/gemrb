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


# GUIW.py - scripts to control some windows from the GUIWORLD winpack
# except of Actions, Portrait, Options and Dialog windows
#################################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows
from GameCheck import MAX_PARTY_SIZE
from GUIDefines import *
from ie_stats import *
import CommonWindow
import Container

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContinueWindow = None

removable_pcs = []

def OpenDialogButton(id):
	window = GemRB.LoadWindow (id, GUICommon.GetWindowPack())
	window.SetFlags (IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)

	MsgWin = GemRB.GetView("MSGWIN")

	frame = MsgWin.GetFrame()
	offset = 0
	if GameCheck.IsGemRBDemo ():
		offset = window.GetFrame()['h']
	window.SetPos(frame['x'], frame['y'] + frame['h'] - offset)

	# Dialog button shouldnt be in front
	win = GemRB.GetView("OPTWIN")
	if win:
		win.Focus()
	win = GemRB.GetView("PORTWIN")
	if win:
		win.Focus()

	return window

def DialogStarted ():
	global ContinueWindow

	# try to force-close anything which is open
	GUICommonWindows.CloseTopWindow()
	Container.CloseContainerWindow()

	# opening control size to maximum, enabling dialog window
	CommonWindow.SetGameGUIHidden(False)
	GemRB.GameSetScreenFlags(GS_DIALOG, OP_OR)

	# disable the 1-6 hotkeys, so they'll work for choosing answers
	if GemRB.GetView ("PORTWIN"):
		GUICommonWindows.UpdatePortraitWindow ()

	ContinueWindow = OpenDialogButton(9)

def DialogEnded ():
	global ContinueWindow

	GUICommonWindows.UpdateActionsWindow()
	if not ContinueWindow:
		return

	# reset the global hotkey, so it doesn't interfere with console use afterwards
	ContinueWindow.GetControl(0).SetHotKey (None, 0, True)
	ContinueWindow.Close ()
	ContinueWindow = None

def CloseContinueWindow ():
	# don't close the actual window now to avoid flickering: we might still want it open
	GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	if not ContinueWindow:
		return

	ContinueWindow.GetControl(0).SetVisible(False)
	ContinueWindow.GetControl(0).SetDisabled(True)

def OpenEndMessageWindow ():
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	EndDLGStrref = 9371
	if GameCheck.IsGemRBDemo ():
		EndDLGStrref = 67
	Button.SetText (EndDLGStrref)
	Button.OnPress (CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_NO_TOOLTIP, OP_OR)
	Button.MakeDefault(True)

def OpenContinueMessageWindow ():
	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetVisible(True)
	Button.SetDisabled(False)
	ContinueStrref = 9372
	if GameCheck.IsGemRBDemo ():
		ContinueStrref = 66
	Button.SetText (ContinueStrref)
	Button.OnPress (CloseContinueWindow)
	Button.SetFlags (IE_GUI_BUTTON_NO_TOOLTIP, OP_OR)
	Button.MakeDefault(True)

def UpdateReformWindow (Window, select):
	need_to_drop = GemRB.GetPartySize ()-MAX_PARTY_SIZE
	if need_to_drop<0:
		need_to_drop = 0

	#excess player number
	Label = Window.GetControl (0x1000000f)
	Label.SetText (str(need_to_drop) )

	#done
	Button = Window.GetControl (8)
	if need_to_drop:
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_ENABLED)

	#remove
	Button = Window.GetControl (15)
	Button.SetText (17507)
	Button.OnPress (lambda: RemovePlayer(select))
	if select:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	PortraitButtons = GUICommonWindows.GetPortraitButtonPairs (Window, 1, "horizontal")
	for i in PortraitButtons:
		Button = PortraitButtons[i]
		if i+1 not in removable_pcs:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)

	for i in removable_pcs:
		index = removable_pcs.index(i)
		if index not in PortraitButtons:
			continue # for saved games with higher party count than the current setup supports
		Button = PortraitButtons[index]
		Button.EnableBorder (FRAME_PC_SELECTED, select == i)
		Portrait = GemRB.GetPlayerPortrait (i, 1)
		pic = Portrait["Sprite"]
		if not pic and not Portrait["ResRef"]:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		Button.SetPicture (pic, "NOPORTSM")
	GUICommonWindows.UpdatePortraitWindow ()
	return

def RemovePlayer (select):
	hideflag = CommonWindow.IsGameGUIHidden()

	wid = 25
	if GameCheck.IsHOW ():
		wid = 0 # at least in guiw08, this is the correct window
	Window = GemRB.LoadWindow (wid, GUICommon.GetWindowPack(), WINDOW_BOTTOM | WINDOW_HCENTER)

	#are you sure
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (17518)

	#confirm
	def RemovePlayerConfirm ():
		if GameCheck.IsBG2():
			GemRB.LeaveParty (select, 2)
		elif GameCheck.IsBG1():
			GemRB.LeaveParty (select, 1)
		else:
			GemRB.LeaveParty (select)

		Window.Close()
		GemRB.GetView ("WIN_REFORM", 0).Close ()
		return

	Button = Window.GetControl (1)
	Button.SetText (17507)
	Button.OnPress (RemovePlayerConfirm)
	Button.MakeDefault()

	#cancel
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.OnPress (Window.Close)
	Button.MakeEscape()

	CommonWindow.SetGameGUIHidden(hideflag)
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenReformPartyWindow ():
	global removable_pcs

	hideflag = CommonWindow.IsGameGUIHidden()

	Window = GemRB.LoadWindow (24, GUICommon.GetWindowPack(), WINDOW_HCENTER|WINDOW_BOTTOM)
	Window.AddAlias ("WIN_REFORM", 0)

	# skip exportable party members (usually only the protagonist)
	removable_pcs = []
	for i in range (1, GemRB.GetPartySize()+1):
		if not GemRB.GetPlayerStat (i, IE_MC_FLAGS)&MC_EXPORTABLE:
			removable_pcs.append(i)

	#PC portraits
	PortraitButtons = GUICommonWindows.GetPortraitButtonPairs (Window, 1, "horizontal")
	for j in PortraitButtons:
		Button = PortraitButtons[j]
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
		color = {'r' : 0, 'g' : 255, 'b' : 0, 'a' : 255}
		Button.SetBorder (FRAME_PC_SELECTED, color, 0, 0, Button.GetInsetFrame(1,1,2,2))
		if j < len(removable_pcs):
			Button.SetValue (removable_pcs[j])
		else:
			Button.SetValue (None)

		Button.OnPress (lambda btn: UpdateReformWindow(Window, btn.Value))

	# Done
	Button = Window.GetControl (8)
	Button.SetText (11973)
	Button.OnPress (Window.Close)

	# if nobody can be removed, just close the window
	if not removable_pcs:
		Window.Close()
		CommonWindow.SetGameGUIHidden(hideflag)
		return

	UpdateReformWindow (Window, None)
	CommonWindow.SetGameGUIHidden(hideflag)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DeathWindow ():
	# break out of any cutscenes, so we get mouse input back
	# can also happen in "cutscene lite" mode, eg. with wk spirit warrior dream #715
	GemRB.EndCutSceneMode ()
	GemRB.ExecuteString ("SetCursorState(0)")
	if GameCheck.IsIWD1():
		#no death movie, but music is changed
		GemRB.LoadMusicPL ("Theme.mus",1)
	CommonWindow.SetGameGUIHidden(True)
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindowEnd ():
	#playing death movie before continuing
	if not GameCheck.IsIWD1():
		GemRB.PlayMovie ("deathand",1)
	GemRB.GamePause (1,3)

	Window = GemRB.LoadWindow (17, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (16498)

	#load
	Button = Window.GetControl (1)
	Button.SetText (15590)
	Button.OnPress (LoadPress)

	#quit
	Button = Window.GetControl (2)
	Button.SetText (15417)
	Button.OnPress (QuitPress)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def QuitPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("Start")
	return

def LoadPress():
	GemRB.QuitGame ()
	GemRB.SetNextScript ("GUILOAD")
	return


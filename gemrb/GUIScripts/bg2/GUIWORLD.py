# -*-python-*-
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
from GUIDefines import *
import GUICommon
import GUICommonWindows
import GUIClasses
from ie_stats import *

FRAME_PC_SELECTED = 0
FRAME_PC_TARGET   = 1

ContinueWindow = None
ReformPartyWindow = None
OldActionsWindow = None
OldMessageWindow = None

removable_pcs = []

def CloseContinueWindow ():
	if ContinueWindow:
		# don't close the actual window now to avoid flickering: we might still want it open
		GemRB.SetVar ("DialogChoose", GemRB.GetVar ("DialogOption"))

def NextDialogState ():
	global ContinueWindow, OldActionsWindow

	if ContinueWindow == None:
		return

	hideflag = GemRB.HideGUI ()

	if ContinueWindow:
		ContinueWindow.Unload ()
	GemRB.SetVar ("ActionsWindow", OldActionsWindow.ID)
	ContinueWindow = None
	OldActionsWindow = None
	if hideflag:
		GemRB.UnhideGUI ()


def OpenEndMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		# try to force-close anything which is open
		GUICommon.CloseOtherWindow(None)

		GemRB.LoadWindowPack (GUICommon.GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GUIClasses.GWindow( GemRB.GetVar ("ActionsWindow") )
		GemRB.SetVar ("ActionsWindow", Window.ID)

	#end dialog
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9371)	
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	if GUICommonWindows.PortraitWindow:
		GUICommonWindows.UpdatePortraitWindow ()
	if hideflag:
		GemRB.UnhideGUI ()


def OpenContinueMessageWindow ():
	global ContinueWindow, OldActionsWindow

	hideflag = GemRB.HideGUI ()

	if not ContinueWindow:
		# try to force-close anything which is open
		GUICommon.CloseOtherWindow(None)

		GemRB.LoadWindowPack (GUICommon.GetWindowPack())
		ContinueWindow = Window = GemRB.LoadWindow (9)
		OldActionsWindow = GUIClasses.GWindow( GemRB.GetVar ("ActionsWindow") )
		GemRB.SetVar ("ActionsWindow", Window.ID)

	#continue
	Button = ContinueWindow.GetControl (0)
	Button.SetText (9372)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseContinueWindow)
	if hideflag:
		GemRB.UnhideGUI ()

def UpdateReformWindow ():
	Window = ReformPartyWindow

	select = GemRB.GetVar ("Selected")

	need_to_drop = GemRB.GetPartySize ()-PARTY_SIZE
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
	if select:
		Button.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		Button.SetState (IE_GUI_BUTTON_DISABLED)

	for i in range (PARTY_SIZE+1):
		Button = Window.GetControl (i)
		if i+1 not in removable_pcs:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue

	for i in removable_pcs:
		Button = Window.GetControl (removable_pcs.index(i))
		Button.EnableBorder (FRAME_PC_SELECTED, select == i )
		pic = GemRB.GetPlayerPortrait (i, 1)
		if not pic:
			Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_SET)
			Button.SetState (IE_GUI_BUTTON_LOCKED)
			continue
		Button.SetState (IE_GUI_BUTTON_ENABLED)
		Button.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_LEFT, OP_SET)
		Button.SetPicture (pic, "NOPORTSM")
	GUICommonWindows.UpdatePortraitWindow ()
	return

def RemovePlayer ():
	global ReformPartyWindow

	hideflag = GemRB.HideGUI ()

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	if ReformPartyWindow:
		ReformPartyWindow.Unload ()
	ReformPartyWindow = Window = GemRB.LoadWindow (25)
	GemRB.SetVar ("OtherWindow", Window.ID)

	#are you sure
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (17518)

	#confirm
	Button = Window.GetControl (1)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RemovePlayerConfirm)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	#cancel
	Button = Window.GetControl (2)
	Button.SetText (13727)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RemovePlayerCancel)
	Button.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	GemRB.SetVar ("OtherWindow", Window.ID)
	GemRB.SetVar ("ActionsWindow", -1)
	if hideflag:
		GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def RemovePlayerConfirm ():
	slot = GemRB.GetVar ("Selected")
	GemRB.LeaveParty (slot, 2)
	OpenReformPartyWindow ()
	return

def RemovePlayerCancel ():
	#Once for getting rid of the confirmation window
	OpenReformPartyWindow ()
	#and once for reopening the reform party window
	OpenReformPartyWindow ()
	return

def OpenReformPartyWindow ():
	global ReformPartyWindow, OldActionsWindow, OldMessageWindow
	global removable_pcs

	GemRB.SetVar ("Selected", 0)
	hideflag = GemRB.HideGUI ()

	if ReformPartyWindow:
		if ReformPartyWindow:
			ReformPartyWindow.Unload ()

		GemRB.SetVar ("ActionsWindow", OldActionsWindow.ID)
		GemRB.SetVar ("MessageWindow", OldMessageWindow.ID)
		GemRB.SetVar ("OtherWindow", -1)

		OldActionsWindow = None
		OldMessageWindow = None
		ReformPartyWindow = None
		if hideflag:
			GemRB.UnhideGUI ()
		#re-enabling party size control
		GemRB.GameSetPartySize (PARTY_SIZE)
		GUICommonWindows.UpdatePortraitWindow()
		return

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	ReformPartyWindow = Window = GemRB.LoadWindow (24)
	GemRB.SetVar ("OtherWindow", Window.ID)

	# skip exportable party members (usually only the protagonist)
	removable_pcs = []
	for i in range (1, GemRB.GetPartySize()+1):
		if not GemRB.GetPlayerStat (i, IE_MC_FLAGS)&MC_EXPORTABLE:
			removable_pcs.append(i)

	#PC portraits
	for j in range (PARTY_SIZE+1):
		Button = Window.GetControl (j)
		Button.SetState (IE_GUI_BUTTON_LOCKED)
		Button.SetFlags (IE_GUI_BUTTON_RADIOBUTTON|IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE,OP_SET)
		Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)
		if j < len(removable_pcs):
			Button.SetVarAssoc ("Selected", removable_pcs[j])
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, UpdateReformWindow)

	# Remove
	Button = Window.GetControl (15)
	Button.SetText (17507)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, RemovePlayer)

	# Done
	Button = Window.GetControl (8)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenReformPartyWindow)

	OldActionsWindow = GUIClasses.GWindow( GemRB.GetVar ("ActionsWindow") )
	OldMessageWindow = GUIClasses.GWindow( GemRB.GetVar ("MessageWindow") )
	GemRB.SetVar ("ActionsWindow", -1)
	GemRB.SetVar ("MessageWindow", -1)

	# if nobody can be removed, just close the window
	if not removable_pcs:
		OpenReformPartyWindow ()
		if hideflag:
			GemRB.UnhideGUI ()
		return

	UpdateReformWindow ()
	if hideflag:
		GemRB.UnhideGUI ()
	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def DeathWindow ():
	GemRB.HideGUI ()
	GemRB.SetTimedEvent (DeathWindowEnd, 10)
	return

def DeathWindowEnd ():
	#playing death movie before continuing
	GemRB.PlayMovie ("deathand",1)
	GemRB.GamePause (1,1)

	GemRB.LoadWindowPack (GUICommon.GetWindowPack())
	Window = GemRB.LoadWindow (17)

	#reason for death
	Label = Window.GetControl (0x0fffffff)
	Label.SetText (16498)

	#load
	Button = Window.GetControl (1)
	Button.SetText (15590)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, LoadPress)

	#quit
	Button = Window.GetControl (2)
	Button.SetText (15417)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, QuitPress)

	GemRB.HideGUI ()
	GemRB.SetVar ("MessageWindow", -1)
	GemRB.UnhideGUI ()
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


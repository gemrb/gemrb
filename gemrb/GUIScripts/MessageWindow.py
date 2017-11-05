# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2003-2005 The GemRB Project
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

# MessageWindow.py - scripts and GUI for the main (walk) window

###################################################

import GemRB
import GameCheck
import GUICommon
import GUICommonWindows

from GUIDefines import *

def OnLoad():
	# just load the medium window always. we can shrink/expand it, but it is the one with both controls
	# this saves us from haveing to bend over backwards to load the new window and move the text to it (its also shorter code)
	# for reference: medium = 12 = guiwdmb8, large = 7 = guwbtp38, small = 4 = guwbtp28
	MessageWindow = GemRB.LoadWindow(12, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MessageWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	MessageWindow.AddAlias("MSGWIN")
	
	TMessageTA = MessageWindow.GetControl(1)
	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	TMessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	TMessageTA.AddAlias("MsgSys", 0)

	ActionsWindow = GemRB.LoadWindow(3, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	GUICommonWindows.OpenActionsWindowControls (ActionsWindow)
	ActionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	ActionsWindow.AddAlias("ACTWIN")
	
	aFrame = ActionsWindow.GetFrame()
	mFrame = MessageWindow.GetFrame()
	MessageWindow.SetPos(mFrame['x'], mFrame['y'] - aFrame['h'])
	
	Button = ActionsWindow.GetControl(60)
	if Button:
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, MaximizeOptions)
		Button=ActionsWindow.GetControl(61)
		Button.SetEvent(IE_GUI_BUTTON_ON_PRESS, MaximizePortraits)
		
	OptionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_LEFT|WINDOW_VCENTER)
	OptionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	OptionsWindow.AddAlias("OPTWIN")
	
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)
	PortraitWindow = GUICommonWindows.OpenPortraitWindow(1)
	PortraitWindow.AddAlias("PORTWIN")

	# 1280 and higher don't have this control
	Button = OptionsWindow.GetControl (10)
	if Button:
		Button = OptionsWindow.GetControl (10)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, MinimizeOptions)
		Button = PortraitWindow.GetControl (8)
		Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUICommonWindows.MinimizePortraits)
	
	UpdateControlStatus()
	
def MinimizeOptions():
	GemRB.GameSetScreenFlags(GS_OPTIONPANE, OP_OR)

def MaximizeOptions():
	GemRB.GameSetScreenFlags(GS_OPTIONPANE, OP_NAND)

# MinimizePortraits is in GUICommonWindows for dependency reasons

def MaximizePortraits():
	GemRB.GameSetScreenFlags(GS_PORTRAITPANE, OP_NAND)

def UpdateControlStatus():
	MessageWindow = GemRB.GetView("MSGWIN")

	if not MessageWindow:
		# exit if we get called from core without the right window pack being loaded
		return

	ExpandButton = MessageWindow.GetControl(0)
	ExpandButton.SetDisabled(False)
	ExpandButton.SetResizeFlags(IE_GUI_VIEW_RESIZE_TOP)
	ExpandButton.SetHotKey(chr(0x8d)) # GEM_PGUP

	ContractButton = MessageWindow.GetControl(3)
	ContractButton.SetFlags(IE_GUI_VIEW_INVISIBLE|IE_GUI_VIEW_DISABLED, OP_NAND)
	ContractButton.SetResizeFlags(IE_GUI_VIEW_RESIZE_BOTTOM)
	ContractButton.SetHotKey(chr(0x8e)) # GEM_PGDOWN
	
	def GetGSFlags():
		GSFlags = GemRB.GetGUIFlags()
		Expand = GSFlags&GS_DIALOGMASK
		GSFlags = GSFlags-Expand
		return (GSFlags, Expand)

	def SetMWSize(size, GSFlags):
		# or if we are going to do this a lot maybe add a view flag for automatically resizing to the assigned background
		WinSizes = {GS_SMALLDIALOG : 45,
					GS_MEDIUMDIALOG : 109,
					GS_LARGEDIALOG : 237}
		
		# FIXME: these are for 800x600. we need to do something like in GUICommon.GetWindowPack()
		WinBG = {GS_SMALLDIALOG : "guwbtp28",
				GS_MEDIUMDIALOG : "guiwdmb8",
				GS_LARGEDIALOG : "guwbtp38"}
		
		if size not in WinSizes:
			return

		frame = MessageWindow.GetFrame()
		diff = frame['h'] - WinSizes[size]
		frame['y'] += diff
		frame['h'] = WinSizes[size]
		MessageWindow.SetFrame(frame)
		MessageWindow.SetBackground(WinBG[size])
		
		frame = ContractButton.GetFrame();
		if size != GS_SMALLDIALOG:
			frame['y'] -= (frame['h'] + 6)
		ExpandButton.SetFrame(frame)

		GemRB.GameSetScreenFlags(size + GSFlags, OP_SET)

	def OnIncreaseSize():
		GSFlags, Expand = GetGSFlags()
		Expand = (Expand + 1)*2 # next size up

		SetMWSize(Expand, GSFlags)

	def OnDecreaseSize():
		GSFlags, Expand = GetGSFlags()
		Expand = Expand/2 - 1 # next size down: 6->2, 2->0

		SetMWSize(Expand, GSFlags)

	ContractButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, OnDecreaseSize)
	ExpandButton.SetEvent(IE_GUI_BUTTON_ON_PRESS, OnIncreaseSize)
	
	GSFlags, Expand = GetGSFlags()

	if Expand == GS_LARGEDIALOG:
		ExpandButton.SetDisabled(True)

	elif (GSFlags&GS_DIALOG):
		#a dialogue is running, setting messagewindow size to maximum
		SetMWSize(GS_LARGEDIALOG, GSFlags)

	elif Expand == GS_SMALLDIALOG:
		ContractButton.SetFlags(IE_GUI_VIEW_INVISIBLE|IE_GUI_VIEW_DISABLED, OP_OR)

	return

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
import ActionsWindow as ActionsWindowModule
import GameCheck
import GUICommon
import GUICommonWindows
import PortraitWindow

from ie_restype import RES_MOS
from GUIDefines import *

def OnLoad():
	# just load the medium window always. we can shrink/expand it, but it is the one with both controls
	# this saves us from having to bend over backwards to load the new window and move the text to it (its also shorter code)
	# for reference: medium = 12 = guiwdmb8, large = 7 = guwbtp38, small = 4 = guwbtp28
	MessageWindow = GemRB.LoadWindow(12, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	MessageWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	MessageWindow.AddAlias("MSGWIN")
	MessageWindow.AddAlias("HIDE_CUT", 0)
	
	TMessageTA = MessageWindow.GetControl(1)
	TMessageTA.SetFlags(IE_GUI_TEXTAREA_AUTOSCROLL|IE_GUI_TEXTAREA_HISTORY)
	TMessageTA.SetResizeFlags(IE_GUI_VIEW_RESIZE_ALL)
	TMessageTA.AddAlias("MsgSys", 0)
	TMessageTA.SetColor(ColorRed, TA_COLOR_OPTIONS)
	TMessageTA.SetColor(ColorWhite, TA_COLOR_HOVER)
	
	sbar = MessageWindow.GetControl(2)
	sbar.SetResizeFlags(IE_GUI_VIEW_RESIZE_VERTICAL)

	ActionsWindow = GemRB.LoadWindow(3, GUICommon.GetWindowPack(), WINDOW_BOTTOM|WINDOW_HCENTER)
	ActionsWindowModule.OpenActionsWindowControls (ActionsWindow)
	ActionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	ActionsWindow.AddAlias("HIDE_CUT", 1)
	ActionsWindow.AddAlias("NOT_DLG", 0)
	
	aFrame = ActionsWindow.GetFrame()
	mFrame = MessageWindow.GetFrame()
	MessageWindow.SetPos(mFrame['x'], mFrame['y'] - aFrame['h'])
	
	if GameCheck.HasHOW():
		OptionsWindow = GemRB.LoadWindow(25, GUICommon.GetWindowPack(), WINDOW_LEFT|WINDOW_VCENTER)
	else:
		OptionsWindow = GemRB.LoadWindow(0, GUICommon.GetWindowPack(), WINDOW_LEFT|WINDOW_VCENTER)
	OptionsWindow.SetFlags(WF_BORDERLESS|IE_GUI_VIEW_IGNORE_EVENTS, OP_OR)
	OptionsWindow.AddAlias("OPTWIN")
	OptionsWindow.AddAlias("HIDE_CUT", 2)
	OptionsWindow.AddAlias("NOT_DLG", 1)
	
	GUICommonWindows.SetupMenuWindowControls (OptionsWindow, 1, None)
	PortraitWin = PortraitWindow.OpenPortraitWindow(1)
	PortraitWin.AddAlias("HIDE_CUT", 3)
	PortraitWin.AddAlias("NOT_DLG", 2)

	Button = ActionsWindow.GetControl (60)
	if Button:
		Button.OnPress (lambda: ToggleWindowMinimize(OptionsWindow, GS_OPTIONPANE))
		Button = ActionsWindow.GetControl (61)
		Button.OnPress (lambda: ToggleWindowMinimize(PortraitWin, GS_PORTRAITPANE))

	# 1280 and higher don't have this control
	Button = OptionsWindow.GetControl (10)
	if Button:
		Button.OnPress (lambda: ToggleWindowMinimize(OptionsWindow, GS_OPTIONPANE))
		Button = PortraitWin.GetControl (8)
		Button.OnPress (lambda: ToggleWindowMinimize(PortraitWin, GS_PORTRAITPANE))

	UpdateControlStatus(True)

# or if we are going to do this a lot maybe add a view flag for automatically resizing to the assigned background
# TODO: add a GUIScript function to return a dict with a CObject<Sprite2D> + dimensions for a given resref
WinSizes = {GS_SMALLDIALOG : 45,
			GS_MEDIUMDIALOG : 109,
			GS_LARGEDIALOG : 237}
if GameCheck.IsAnyEE ():
	WinSizes = {GS_SMALLDIALOG : 50,
			GS_MEDIUMDIALOG : 141,
			GS_LARGEDIALOG : 300}

tries = 0
def MWinBG(size, pack = None):
	global tries

	if pack is None:
		pack = GUICommon.GetWindowPack ()

	bg = None
	if pack == "GUIW":
		bgs = { GS_SMALLDIALOG: "guiwbtp2", GS_MEDIUMDIALOG: "guiwdmb", GS_LARGEDIALOG: "guiwbtp3" }
		bg = bgs[size]
	else:
		bgs = { GS_SMALLDIALOG: "guwbtp2", GS_MEDIUMDIALOG: "guiwdmb", GS_LARGEDIALOG: "guwbtp3" }
		bg = bgs[size]

		if pack == "GUIW08":
			bg = bg + "8"
		elif pack == "GUIW10":
			bg = bg + "0"
		elif GameCheck.IsAnyEE (): # "GUIW13" ... "GUIW20"
			bg = bg + "9" # 1160px wide
		else:
			raise ValueError("Unexpected window pack: " + pack)

	if GemRB.HasResource (bg, RES_MOS):
		tries = 0
	else:
		tries += 1
		if tries > 1:
			raise RuntimeError("Cannot find message window background for " + pack)
		if GameCheck.IsBG2OrEE ():
			return MWinBG(size, "GUIW08")
		else:
			return MWinBG(size, "GUIW")

	return bg

MinimizedWindows = {}
def ToggleWindowMinimize(win, GSFlag = 0):
	key = win.ID
	maximize = True if key in MinimizedWindows else False
	if maximize:
		# restore to original size
		win.SetSize(*MinimizedWindows[key])
		del MinimizedWindows[key]

		if GSFlag: # stored in save
			GemRB.GameSetScreenFlags(GSFlag, OP_NAND)

	else: # minimize
		size = win.GetSize()
		MinimizedWindows[key] = size
		win.SetSize(size[0], 5)
		win.ScrollTo(0,0)

		if GSFlag: # stored in save
			GemRB.GameSetScreenFlags(GSFlag, OP_OR)
			
	if key == GemRB.GetView("OPTWIN").ID:
		ToggleActionbarClock(not maximize)

def ToggleActionbarClock(show):
	actwin = GemRB.GetView("ACTWIN")
	clock = actwin.GetControl (62)
	if clock:
		clock.SetVisible(show)

MTARestoreSize = None
def UpdateControlStatus(init = False):
	MessageWindow = GemRB.GetView("MSGWIN")

	if not MessageWindow:
		# exit if we get called from core without the right window pack being loaded
		return

	ExpandButton = MessageWindow.GetControl(0)
	ExpandButton.SetDisabled(False)
	ExpandButton.SetResizeFlags(IE_GUI_VIEW_RESIZE_TOP)
	ExpandButton.SetHotKey(chr(0x8d), 0, True) # GEM_PGUP

	ContractButton = MessageWindow.GetControl(3)
	ContractButton.SetFlags(IE_GUI_VIEW_INVISIBLE|IE_GUI_VIEW_DISABLED, OP_NAND)
	ContractButton.SetResizeFlags(IE_GUI_VIEW_RESIZE_BOTTOM)
	ContractButton.SetHotKey(chr(0x8e), 0, True) # GEM_PGDOWN
	
	def GetGSFlags():
		GSFlags = GemRB.GetGUIFlags()
		Expand = GSFlags&GS_DIALOGMASK
		GSFlags = GSFlags-Expand
		return (GSFlags, Expand)

	def SetMWSize(size, GSFlags):
		if size not in WinSizes:
			return
		
		frame = ContractButton.GetFrame()
		if size != GS_SMALLDIALOG:
			frame['y'] -= (frame['h'] + 6)
		ExpandButton.SetFrame(frame)

		frame = MessageWindow.GetFrame()
		diff = frame['h'] - WinSizes[size]
		frame['y'] += diff
		frame['h'] = WinSizes[size]
		MessageWindow.SetFrame(frame)
		MessageWindow.SetBackground(MWinBG(size))

		GemRB.GameSetScreenFlags(size + GSFlags, OP_SET)

	def OnIncreaseSize():
		GSFlags, Expand = GetGSFlags()
		Expand = (Expand + 1)*2 # next size up

		SetMWSize(Expand, GSFlags)

	def OnDecreaseSize():
		GSFlags, Expand = GetGSFlags()
		Expand = Expand // 2 - 1 # next size down: 6->2, 2->0

		SetMWSize(Expand, GSFlags)

	global MTARestoreSize
	ContractButton.OnPress (OnDecreaseSize)
	ExpandButton.OnPress (OnIncreaseSize)

	GSFlags, Expand = GetGSFlags()
	if init:
		SetMWSize(Expand, GSFlags)
		if GSFlags&GS_OPTIONPANE:
			win = GemRB.GetView("OPTWIN")
			ToggleWindowMinimize(win)
		else:
			ToggleActionbarClock(False)
		if GSFlags&GS_PORTRAITPANE:
			win = GemRB.GetView("PORTWIN")
			ToggleWindowMinimize(win)
	else:
		if Expand == GS_LARGEDIALOG:
			if MTARestoreSize is not None and (GSFlags&GS_DIALOG) == 0:
				SetMWSize(MTARestoreSize, GSFlags)
				MTARestoreSize = None
			else:
				ExpandButton.SetDisabled(True)

		elif (GSFlags&GS_DIALOG):
			MTARestoreSize = Expand
			#a dialogue is running, setting messagewindow size to maximum
			SetMWSize(GS_LARGEDIALOG, GSFlags)

		elif Expand == GS_SMALLDIALOG:
			ContractButton.SetFlags(IE_GUI_VIEW_INVISIBLE|IE_GUI_VIEW_DISABLED, OP_OR)

	return

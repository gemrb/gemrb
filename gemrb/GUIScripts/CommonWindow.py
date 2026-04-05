# SPDX-FileCopyrightText: 2010 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

import GemRB
import GUIClasses
from GUIDefines import *

def SetGameGUIHidden(hide):
	op = OP_OR if hide else OP_NAND
	GemRB.GameSetScreenFlags(GS_HIDEGUI, op)

def IsGameGUIHidden():
	return GemRB.GetGUIFlags() & GS_HIDEGUI

# for keymap.2da
def ToggleGUIHidden():
	SetGameGUIHidden(not IsGameGUIHidden())
	
def AddScrollbarProxy(win, sbar, leftctl):
	frame = sbar.GetFrame()
	
	ctlFrame = leftctl.GetFrame()
	frame['w'] = frame['x'] - ctlFrame['x']
	frame['x'] = ctlFrame['x']
	
	scrollview = GemRB.CreateView(AddScrollbarProxy.proxyID, IE_GUI_VIEW, frame)
	AddScrollbarProxy.proxyID += 1
	scrollview = win.AddSubview(scrollview, win.GetControl(99)) # just something behind all the buttons and labels
	scrollview.SetEventProxy(sbar)
	
AddScrollbarProxy.proxyID = 1000

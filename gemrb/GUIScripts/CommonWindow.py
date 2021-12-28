# GemRB - Infinity Engine Emulator
# Copyright (C) 2010 The GemRB Project
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
	
def AddScrollbarProxy(win, sbar, xcoord):
	frame = sbar.GetFrame()
	
	# FIXME: PST seems to have scrollbars on the left occasionally, but we dont handle that here
	# This proxy function wont work for those cases
	frame['w'] = frame['x'] - xcoord
	frame['x'] = xcoord
	
	scrollview = GemRB.CreateView(AddScrollbarProxy.proxyID, IE_GUI_VIEW, frame)
	AddScrollbarProxy.proxyID += 1
	scrollview = win.AddSubview(scrollview, win.GetControl(99)) # just something behind all the buttons and labels
	scrollview.SetEventProxy(sbar)
	
AddScrollbarProxy.proxyID = 1000

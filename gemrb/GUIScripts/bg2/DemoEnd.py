# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2007 The GemRB Project
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

# DemoEnd.py - display DemoEnd pictures

###################################################

import GemRB

Picture = None
Table = None
Window = None

def OnLoad ():
	global Table, Picture, Window
	
	GemRB.LoadWindowPack("demoend", 640, 480)
	Window = GemRB.LoadWindow(0)
	Picture = 0
	Table = GemRB.LoadTable ("splashsc")
	resref = Table.GetValue (Picture,0)
	Button = Window.GetControl (0)
	Button.SetFlags (IE_GUI_BUTTON_DEFAULT|IE_GUI_BUTTON_CANCEL|IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, NextPress)
	Button.SetPicture (resref)
	Window.SetVisible (WINDOW_VISIBLE)
	return
	
def NextPress ():
	global Picture
	
	Picture = Picture + 1
	if Table.GetRowCount()<=Picture:
		DemoEnd()
	else:
		resref = Table.GetValue (Picture,0)
		Button = Window.GetControl (0)
		Button.SetPicture (resref)
	return
	
def DemoEnd ():
	GemRB.SetNextScript ("Start")
	return

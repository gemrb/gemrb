# SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# DemoEnd.py - display DemoEnd pictures

###################################################

import GemRB

Picture = None
Table = None
Window = None

def OnLoad ():
	global Table, Picture, Window
	
	Window = GemRB.LoadWindow(0, "demoend")
	Picture = 0
	Table = GemRB.LoadTable ("splashsc")
	resref = Table.GetValue (Picture,0)
	Button = Window.GetControl (0)
	Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_SET)
	Button.MakeDefault()
	Button.SetState (IE_GUI_BUTTON_LOCKED)
	Button.OnPress (NextPress)
	Button.SetPicture (resref)
	Window.Focus()
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

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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
#
#character generation, biography (GUICG23)
import GemRB

BioWindow = 0
EditControl = 0

def OnLoad ():
	global BioWindow, EditControl

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	BioWindow = GemRB.LoadWindow (23)

	EditControl = BioWindow.GetControl (3)
	BIO = GemRB.GetToken("BIO")
	EditTextArea = BioWindow.CreateTextArea(100, 0, 0, 0, 0, "NORMAL", IE_FONT_ALIGN_CENTER) # ID/position/size dont matter. we will substitute later
	EditControl = EditTextArea.SubstituteForControl (EditControl)
	EditControl.SetVarAssoc ("row", 0)
	EditControl.SetStatus (IE_GUI_CONTROL_FOCUSED)
	if BIO:
		EditControl.SetText (BIO)
	else:
		EditControl.SetText (15882)

	# done
	OkButton = BioWindow.GetControl (1)
	OkButton.SetText (11973)

	ClearButton = BioWindow.GetControl (4)
	ClearButton.SetText (34881)

	# back
	CancelButton = BioWindow.GetControl (2)
	CancelButton.SetText (12896)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	OkButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OkPress)
	ClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClearPress)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)
	BioWindow.SetVisible (WINDOW_VISIBLE)
	return

def OkPress ():
	global BioWindow, EditControl

	row = 0
	line = None
	BioData = ""

	#there is no way to get the entire TextArea content
	#this hack retrieves the TextArea content row by row
	#there is no way to know how much data is in the TextArea
	while 1:
		GemRB.SetVar ("row", row)
		EditControl.SetVarAssoc ("row", row)
		line = EditControl.QueryText ()
		if len(line)<=0:
			break
		BioData += line+"\n"
		row += 1
	
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	GemRB.SetToken ("BIO", BioData)
	return
	
def CancelPress ():
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	return

def ClearPress ():
	EditControl.SetText ("")
	return

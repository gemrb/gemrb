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

def OnLoad ():
	global BioWindow

	BioWindow = GemRB.LoadWindow (23, "GUICG")

	placeholder = BioWindow.GetControl (3)
	BIO = GemRB.GetToken("BIO")
	EditTextArea = BioWindow.CreateTextArea(100, 0, 0, 0, 0, "NORMAL")
	EditTextArea.SetFrame(placeholder.GetFrame())
	EditTextArea.AddAlias("BIO")
	BioWindow.DeleteControl(placeholder)

	if BIO:
		EditTextArea.SetText (BIO)
	else:
		EditTextArea.SetText (15882)

	# done
	OkButton = BioWindow.GetControl (1)
	OkButton.SetText (11973)

	ClearButton = BioWindow.GetControl (4)
	ClearButton.SetText (34881)

	# back
	CancelButton = BioWindow.GetControl (2)
	CancelButton.SetText (12896)
	CancelButton.MakeEscape()

	OkButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OkPress)
	ClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, lambda: EditTextArea.Clear())
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)
	return

def OkPress ():
	global BioWindow

	row = 0
	line = None
	TA = GemRB.GetView("BIO")
	BioData = TA.QueryText ()
	GemRB.SetToken ("BIO", BioData)
	
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	return
	
def CancelPress ():
	if BioWindow:
		BioWindow.Unload ()
	GemRB.SetNextScript ("CharGen9")
	return

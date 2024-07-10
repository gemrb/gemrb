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
import CharGenCommon

BioWindow = 0

def OnLoad ():
	global BioWindow

	BioWindow = GemRB.LoadWindow (23, "GUICG")
	CharGenCommon.PositionCharGenWin(BioWindow)
	BioWindow.DeleteControl (5)

	EditTextArea = BioWindow.ReplaceSubview(3, IE_GUI_TEXTAREA, "NORMAL")
	BIO = GemRB.GetToken("BIO")
	EditTextArea.AddAlias("BIO")
	EditTextArea.SetFlags(IE_GUI_TEXTAREA_EDITABLE, OP_OR)
	EditTextArea.SetColor (ColorWhitish, TA_COLOR_NORMAL)

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

	OkButton.OnPress (OkPress)
	ClearButton.OnPress (ClearBiography)
	CancelButton.OnPress (CancelPress)
	EditTextArea.Focus ()
	return

def ClearBiography():
	EditTextArea = GemRB.GetView ("BIO")
	EditTextArea.Clear ()
	EditTextArea.Focus ()

def OkPress ():
	global BioWindow

	TA = GemRB.GetView("BIO")
	BioData = TA.QueryText ()
	GemRB.SetToken ("BIO", BioData)
	
	if BioWindow:
		BioWindow.Close ()
	GemRB.SetNextScript ("CharGen9")
	return
	
def CancelPress ():
	if BioWindow:
		BioWindow.Close ()
	GemRB.SetNextScript ("CharGen9")
	return

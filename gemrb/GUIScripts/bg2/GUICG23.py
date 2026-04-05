# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

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

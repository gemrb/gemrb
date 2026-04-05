# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

#character generation, import (GUICG24)
import GemRB

ImportWindow = 0

def OnLoad():
	global ImportWindow

	ImportWindow = GemRB.LoadWindow(24, "GUICG")
	ImportWindow.SetFlags (WF_ALPHA_CHANNEL, OP_OR)

	TextAreaControl = ImportWindow.GetControl(0)
	TextAreaControl.SetText(53605)

	FileButton = ImportWindow.GetControl(1)
	FileButton.SetText(53604)

	SavedGameButton = ImportWindow.GetControl(2)
	SavedGameButton.SetText(53602)

	CancelButton = ImportWindow.GetControl(3)
	CancelButton.SetText(13727)
	CancelButton.MakeEscape()

	FileButton.OnPress (FilePress)
	SavedGameButton.OnPress (GamePress)
	CancelButton.OnPress (CancelPress)
	ImportWindow.ShowModal(MODAL_SHADOW_GRAY)
	return

def FilePress():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("ImportFile")
	return
	
def GamePress():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("ImportGame")
	return

def CancelPress():
	if ImportWindow:
		ImportWindow.Close ()
	GemRB.SetNextScript("CharGen")
	return

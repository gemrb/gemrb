# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2011 The GemRB Project
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
# code shared between the common GUIREC and that of iwd2 (pst)
import GemRB
import GameCheck
import GUICommon
import Portrait
from GUIDefines import *
from ie_stats import IE_SEX, IE_CLASS, IE_RACE, IE_MC_FLAGS, MC_EXPORTABLE
from ie_restype import RES_WAV

BiographyWindow = None
CustomizeWindow = None
SubCustomizeWindow = None
SubSubCustomizeWindow = None
ExportWindow = None
NameField = ExportDoneButton = None
ScriptsTable = None
RevertButton = None

if GameCheck.IsBG2() or GameCheck.IsBG1():
	BioStrRefSlot = 74
else:
	BioStrRefSlot = 63

if GameCheck.IsBG2() or GameCheck.IsIWD2():
	PortraitNameSuffix = "L"
else:
	PortraitNameSuffix = "G"

PortraitPictureButton = None
PortraitList1 = PortraitList2 = RowCount1 = RowCount2 = None

# the available sounds
if GameCheck.IsIWD1() or GameCheck.IsIWD2():
	SoundSequence = [ '01', '02', '03', '04', '05', '06', '07', '08', '09', '10', '11', '12', \
		'13', '14', '15', '16', '17', '18', '19', '20', '21', '22', '23', '24', \
		'25', '26', '27', '28', '29', '30', '31']
else:
	SoundSequence = [ 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', \
		'm', 's', 't', 'u', 'v', '_', 'x', 'y', 'z', '0', '1', '2', \
		'3', '4', '5', '6', '7', '8', '9']
SoundIndex = 0
VoiceList = None
OldVoiceSet = None
Gender = None

def OpenCustomizeWindow ():
	import GUIREC
	global CustomizeWindow, ScriptsTable, Gender

	pc = GemRB.GameGetSelectedPCSingle ()
	if GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE:
		Exportable = 1
	else:
		Exportable = 0

	ScriptsTable = GemRB.LoadTable ("SCRPDESC")
	GUIREC.ColorTable = GemRB.LoadTable ("CLOWNCOL")
	Gender = GemRB.GetPlayerStat (pc, IE_SEX)
	CustomizeWindow = GemRB.LoadWindow (17)

	PortraitSelectButton = CustomizeWindow.GetControl (0)
	PortraitSelectButton.SetText (11961)
	if not Exportable:
		PortraitSelectButton.SetState (IE_GUI_BUTTON_DISABLED)

	SoundButton = CustomizeWindow.GetControl (1)
	SoundButton.SetText (10647)
	if not Exportable:
		SoundButton.SetState (IE_GUI_BUTTON_DISABLED)

	if not GameCheck.IsIWD2():
		ColorButton = CustomizeWindow.GetControl (2)
		ColorButton.SetText (10646)
		ColorButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, GUIREC.OpenColorWindow)
		if not Exportable:
			ColorButton.SetState (IE_GUI_BUTTON_DISABLED)

	ScriptButton = CustomizeWindow.GetControl (3)
	ScriptButton.SetText (17111)

	#This button does not exist in bg1 and pst, but theoretically we could create it here
	if not (GameCheck.IsBG1() or GameCheck.IsPST()):
		BiographyButton = CustomizeWindow.GetControl (9)
		BiographyButton.SetText (18003)
		BiographyButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenBiographyEditWindow)
		if not Exportable:
			BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	TextArea = CustomizeWindow.GetControl (5)
	TextArea.SetText (11327)

	CustomizeDoneButton = CustomizeWindow.GetControl (7)
	CustomizeDoneButton.SetText (11973)
	CustomizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	CancelButton = CustomizeWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PortraitSelectButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenPortraitSelectWindow)
	SoundButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenSoundWindow)
	ScriptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenScriptWindow)
	CustomizeDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CustomizeDonePress)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CustomizeCancelPress)

	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CustomizeDonePress ():
	CloseCustomizeWindow ()
	return

def CustomizeCancelPress ():
	CloseCustomizeWindow ()
	return

def CloseCustomizeWindow ():
	import GUIREC
	global CustomizeWindow
	if CustomizeWindow:
		CustomizeWindow.Unload ()
		CustomizeWindow = None
		GUIREC.UpdateRecordsWindow ()
	return

def OpenPortraitSelectWindow ():
	global PortraitPictureButton, SubCustomizeWindow

	SubCustomizeWindow = GemRB.LoadWindow (18)

	PortraitPictureButton = SubCustomizeWindow.GetControl (0)
	PortraitPictureButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitPictureButton.SetState (IE_GUI_BUTTON_LOCKED)

	PortraitLeftButton = SubCustomizeWindow.GetControl (1)
	PortraitLeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitLeftButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitLeftPress)

	PortraitRightButton = SubCustomizeWindow.GetControl (2)
	PortraitRightButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitRightButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitRightPress)

	PortraitDoneButton = SubCustomizeWindow.GetControl (3)
	PortraitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PortraitDonePress)
	PortraitDoneButton.SetText (11973)
	PortraitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	PortraitCancelButton = SubCustomizeWindow.GetControl (4)
	PortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseSubCustomizeWindow)
	PortraitCancelButton.SetText (13727)
	PortraitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PortraitCustomButton = SubCustomizeWindow.GetControl (5)
	PortraitCustomButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCustomButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, OpenCustomPortraitWindow)
	PortraitCustomButton.SetText (17545)

	# get players gender and portrait
	Pc = GemRB.GameGetSelectedPCSingle ()
	PcPortrait = GemRB.GetPlayerPortrait(Pc,0)

	# initialize and set portrait
	Portrait.Init (Gender)
	Portrait.Set (PcPortrait)
	PortraitPictureButton.SetPicture (Portrait.Name () + PortraitNameSuffix, "NOPORTLG")

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def PortraitDonePress ():
	pc = GemRB.GameGetSelectedPCSingle ()
	# eh, different sizes
	if GameCheck.IsBG2():
		GemRB.FillPlayerInfo (pc, Portrait.Name () + "M", Portrait.Name () + "S")
	else:
		GemRB.FillPlayerInfo (pc, Portrait.Name () + "L", Portrait.Name () + "S")
	CloseSubCustomizeWindow ()
	return

def PortraitLeftPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Previous () + PortraitNameSuffix, "NOPORTLG")

def PortraitRightPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Next () + PortraitNameSuffix, "NOPORTLG")

def OpenCustomPortraitWindow ():
	global SubSubCustomizeWindow
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2

	SubSubCustomizeWindow = GemRB.LoadWindow (19)

	CustomPortraitDoneButton = SubSubCustomizeWindow.GetControl (10)
	CustomPortraitDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	CustomPortraitDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CustomPortraitDonePress)
	CustomPortraitDoneButton.SetText (11973)
	CustomPortraitDoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CustomPortraitCancelButton = SubSubCustomizeWindow.GetControl (11)
	CustomPortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CustomPortraitCancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseSubSubCustomizeWindow)
	CustomPortraitCancelButton.SetText (13727)
	CustomPortraitCancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	if not GameCheck.IsIWD1():
		SmallPortraitButton = SubSubCustomizeWindow.GetControl (1)
		SmallPortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
		LargePortraitButton = SubSubCustomizeWindow.GetControl (0)
		LargePortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# Portrait List Large
	PortraitList1 = SubSubCustomizeWindow.GetControl (2)
	RowCount1 = PortraitList1.GetPortraits (0)
	PortraitList1.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, LargeCustomPortrait)
	GemRB.SetVar ("Row1", RowCount1)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	# Portrait List Small
	PortraitList2 = SubSubCustomizeWindow.GetControl (3)
	RowCount2 = PortraitList2.GetPortraits (1)
	PortraitList2.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, SmallCustomPortrait)
	GemRB.SetVar ("Row2", RowCount2)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	SubSubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CustomPortraitDonePress ():
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.FillPlayerInfo (pc, PortraitList1.QueryText () , PortraitList2.QueryText ())

	CloseSubSubCustomizeWindow ()
	#closing the generic portraits, because we just set a custom one
	CloseSubCustomizeWindow ()
	return

def LargeCustomPortrait ():
	Window = SubSubCustomizeWindow

	Portrait = PortraitList1.QueryText ()
	#small hack
	if GemRB.GetVar ("Row1") == RowCount1:
		return

	Label = Window.GetControl (0x10000007)
	Label.SetText (Portrait)

	Button = Window.GetControl (10)
	if Portrait=="":
		Portrait = "NOPORTMD"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList2.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (0)
	Button.SetPicture (Portrait, "NOPORTMD")
	return

def SmallCustomPortrait ():
	Window = SubSubCustomizeWindow

	Portrait = PortraitList2.QueryText ()
	#small hack
	if GemRB.GetVar ("Row2") == RowCount2:
		return

	Label = Window.GetControl (0x10000008)
	Label.SetText (Portrait)

	Button = Window.GetControl (10)
	if Portrait=="":
		Portrait = "NOPORTSM"
		Button.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		if PortraitList1.QueryText ()!="":
			Button.SetState (IE_GUI_BUTTON_ENABLED)

	Button = Window.GetControl (1)
	Button.SetPicture (Portrait, "NOPORTSM")
	return

def OpenSoundWindow ():
	global VoiceList, OldVoiceSet
	global SubCustomizeWindow

	pc = GemRB.GameGetSelectedPCSingle ()
	OldVoiceSet = GemRB.GetPlayerSound (pc)
	SubCustomizeWindow = GemRB.LoadWindow (20)

	VoiceList = SubCustomizeWindow.GetControl (5)
	VoiceList.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)

	VoiceList.SetVarAssoc ("Selected", 0)
	VoiceList.GetCharSounds()
	VoiceList.SelectText (OldVoiceSet)

	PlayButton = SubCustomizeWindow.GetControl (7)
	PlayButton.SetText (17318)

	TextArea = SubCustomizeWindow.GetControl (8)
	TextArea.SetText (11315)

	DoneButton = SubCustomizeWindow.GetControl (10)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (11)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	PlayButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, PlaySoundPressed)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DoneSoundWindow)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseSoundWindow)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseSoundWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerSound (pc, OldVoiceSet)
	CloseSubCustomizeWindow ()
	return

def DoneSoundWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	CharSound = VoiceList.QueryText ()
	GemRB.SetPlayerSound (pc, CharSound)

	CloseSubCustomizeWindow ()
	return

def PlaySoundPressed():
	global SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText ()
	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		pc = GemRB.GameGetSelectedPCSingle ()
		GemRB.SetPlayerSound (pc, CharSound)
		VoiceSet = GemRB.GetPlayerSound (pc, 1)
	else:
		VoiceSet = CharSound
	tmp = SoundIndex
	while (not GemRB.HasResource (VoiceSet + SoundSequence[SoundIndex], RES_WAV)):
		NextSound()
		if SoundIndex == tmp:
			break
	else:
		NextSound()
	GemRB.PlaySound (VoiceSet + SoundSequence[SoundIndex], 0, 0, 5)
	return

def NextSound():
	global SoundIndex, SoundSequence
	SoundIndex += 1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0
	return

def OpenScriptWindow ():
	global SubCustomizeWindow
	global ScriptTextArea, SelectedTextArea

	SubCustomizeWindow = GemRB.LoadWindow (11)

	ScriptTextArea = SubCustomizeWindow.GetControl (2)
	ScriptTextArea.SetFlags (IE_GUI_TEXTAREA_SELECTABLE)
	FillScriptList ()
	pc = GemRB.GameGetSelectedPCSingle ()
	script = GemRB.GetPlayerScript (pc)
	scriptindex = ScriptsTable.GetRowIndex (script)
	GemRB.SetVar ("Selected", scriptindex)
	ScriptTextArea.SetVarAssoc ("Selected", scriptindex)

	SelectedTextArea = SubCustomizeWindow.GetControl (4)
	UpdateScriptSelection ()

	DoneButton = SubCustomizeWindow.GetControl (5)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	CancelButton = SubCustomizeWindow.GetControl (6)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DoneScriptWindow)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseSubCustomizeWindow)
	ScriptTextArea.SetEvent (IE_GUI_TEXTAREA_ON_CHANGE, UpdateScriptSelection)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def FillScriptList ():
	global ScriptTextArea

	ScriptTextArea.Clear ()
	row = ScriptsTable.GetRowCount ()
	for i in range (row):
		GemRB.SetToken ("script", ScriptsTable.GetRowName (i) )
		title = ScriptsTable.GetValue (i,0)
		if title!=-1:
			desc = ScriptsTable.GetValue (i,1)
			txt = GemRB.GetString (title)
			if (desc!=-1):
				txt += GemRB.GetString (desc)
			ScriptTextArea.Append (txt+"\n", -1)
		else:
			ScriptTextArea.Append (ScriptsTable.GetRowName (i)+"\n" ,-1)
	return

def DoneScriptWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	script = ScriptsTable.GetRowName (GemRB.GetVar ("Selected") )
	GemRB.SetPlayerScript (pc, script)
	CloseSubCustomizeWindow ()
	return

def UpdateScriptSelection():
	text = ScriptTextArea.QueryText ()
	SelectedTextArea.SetText (text)
	return

def RevertBiography():
	global BioStrRef
	global RevertButton

	if GameCheck.IsIWD2():
		BioTable = GemRB.LoadTable ("bios")
		pc = GemRB.GameGetSelectedPCSingle ()
		ClassName = GUICommon.GetClassRowName (pc)
		BioStrRef = BioTable.GetValue (ClassName, "BIO")
	else:
		BioStrRef = 33347
	TextArea.SetText (BioStrRef)
	RevertButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def OpenBiographyEditWindow ():
	global SubCustomizeWindow
	global RevertButton
	global BioStrRef
	global TextArea

	Changed = 0
	pc = GemRB.GameGetSelectedPCSingle ()
	BioStrRef = GemRB.GetPlayerString (pc, BioStrRefSlot)
	if BioStrRef != 33347:
		Changed = 1

	# 23 and 24 were deleted and replaced in iwd
	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		SubCustomizeWindow = GemRB.LoadWindow (51)
	else:
		SubCustomizeWindow = GemRB.LoadWindow (23)

	ClearButton = SubCustomizeWindow.GetControl (5)
	if GameCheck.IsBG2():
		ClearButton.SetText (34881)
	else:
		ClearButton.SetText (18622)

	DoneButton = SubCustomizeWindow.GetControl (1)
	DoneButton.SetText (11973)
	DoneButton.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)

	ScrollbarID = 6
	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		RevertButton = SubCustomizeWindow.GetControl (6)
		ScrollbarID = 3
	else:
		RevertButton = SubCustomizeWindow.GetControl (3)
	RevertButton.SetText (2240)
	if not Changed:
		RevertButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = SubCustomizeWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	TextArea = SubCustomizeWindow.GetControl (4)
	TextArea = TextArea.ConvertEdit (ScrollbarID)
	TextArea.SetText (BioStrRef)

	ClearButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ClearBiography)
	DoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, DoneBiographyWindow)
	RevertButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, RevertBiography)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseSubCustomizeWindow)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	TextArea.SetStatus (IE_GUI_CONTROL_FOCUSED) # DO NOT MOVE near the rest of TextArea handling
	return

def ClearBiography():
	global BioStrRef
	global RevertButton

	pc = GemRB.GameGetSelectedPCSingle ()
	#pc is 1 based
	BioStrRef = 62015+pc
	TextArea.SetText ("")
	RevertButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DoneBiographyWindow ():
	global BioStrRef

	pc = GemRB.GameGetSelectedPCSingle ()
	if BioStrRef != 33347:
		GemRB.CreateString (BioStrRef, TextArea.QueryText())
	GemRB.SetPlayerString (pc, BioStrRefSlot, BioStrRef)
	CloseSubCustomizeWindow ()
	return

def OpenBiographyWindow ():
	global BiographyWindow

	if BiographyWindow != None:
		CloseBiographyWindow ()
		return

	BiographyWindow = Window = GemRB.LoadWindow (12)

	TextArea = Window.GetControl (0)
	pc = GemRB.GameGetSelectedPCSingle ()
	if GameCheck.IsBG1 () and GemRB.GetPlayerName (pc, 2) == 'none':
		TextArea.SetText (GetProtagonistBiography (pc))
	else:
		TextArea.SetText (GemRB.GetPlayerString (pc, BioStrRefSlot))

	# Done
	Button = Window.GetControl (2)
	Button.SetText (11973)
	Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, CloseBiographyWindow)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseBiographyWindow ():
	import GUIREC
	global BiographyWindow

	if BiographyWindow:
		BiographyWindow.Unload ()
		BiographyWindow = None
	# TODO: check if bg1 wouldn't benefit from modality too
	if GUIREC.InformationWindow:
		if GameCheck.IsBG2() or GameCheck.IsIWD1():
			GUIREC.InformationWindow.ShowModal (MODAL_SHADOW_GRAY)
		else:
			GUIREC.InformationWindow.SetVisible (WINDOW_VISIBLE)
	return

def GetProtagonistBiography (pc):
	BioTable = GemRB.LoadTable ("bios")
	racestrings = [ 15895, 15891, 15892, 15890, 15893, 15894 ]

	ClassName = GUICommon.GetClassRowName (pc)
	bio = GemRB.GetString (BioTable.GetValue (ClassName, "BIO"))
	race = GemRB.GetPlayerStat (pc, IE_RACE)
	if race <= 6:
		bio += "\n\n" + GemRB.GetString (racestrings[race-1])
	return bio

def OpenExportWindow ():
	global ExportWindow, NameField, ExportDoneButton

	ExportWindow = GemRB.LoadWindow (13)

	TextArea = ExportWindow.GetControl (2)
	TextArea.SetText (10962)

	TextArea = ExportWindow.GetControl (0)
	TextArea.GetCharacters ()

	ExportDoneButton = ExportWindow.GetControl (4)
	ExportDoneButton.SetText (11973)
	ExportDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = ExportWindow.GetControl (5)
	CancelButton.SetText (13727)
	CancelButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	NameField = ExportWindow.GetControl (6)

	ExportDoneButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExportDonePress)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ExportCancelPress)
	NameField.SetEvent (IE_GUI_EDIT_ON_CHANGE, ExportEditChanged)
	ExportWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.SetStatus (IE_GUI_CONTROL_FOCUSED)
	return

def ExportDonePress():
	#save file under name from EditControl
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SaveCharacter (pc, NameField.QueryText ())
	if ExportWindow:
		ExportWindow.Unload()
	return

def ExportCancelPress():
	global ExportWindow
	if ExportWindow:
		ExportWindow.Unload()
		ExportWindow = None
	return

def ExportEditChanged():
	ExportFileName = NameField.QueryText ()
	if ExportFileName == "":
		ExportDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		ExportDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def CloseSubCustomizeWindow ():
	global SubCustomizeWindow, CustomizeWindow

	if SubCustomizeWindow:
		SubCustomizeWindow.Unload ()
		SubCustomizeWindow = None
	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseSubSubCustomizeWindow ():
	global SubSubCustomizeWindow, SubCustomizeWindow

	if SubSubCustomizeWindow:
		SubSubCustomizeWindow.Unload ()
		SubSubCustomizeWindow = None
	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

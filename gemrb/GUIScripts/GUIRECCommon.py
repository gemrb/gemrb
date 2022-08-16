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
import collections
import GemRB
import GameCheck
import GUICommon
import Portrait
import PaperDoll

from GUIDefines import *
from ie_stats import IE_SEX, IE_RACE, IE_MC_FLAGS, MC_EXPORTABLE
from ie_restype import RES_WAV

CustomizeWindow = None
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
# two bg1 default soundsets and nothing else
SoundSequence2 = [ "03", "08", "09", "10", "11", "17", "18", "19", "20", "21", "22", "38", "39" ]
SoundIndex = 0
VoiceList = None
OldVoiceSet = None
Gender = None

def OpenCustomizeWindow ():
	global CustomizeWindow, ScriptsTable, Gender

	pc = GemRB.GameGetSelectedPCSingle ()
	if GemRB.GetPlayerStat (pc, IE_MC_FLAGS)&MC_EXPORTABLE:
		Exportable = 1
	else:
		Exportable = 0

	ScriptsTable = GemRB.LoadTable ("SCRPDESC")
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
		ColorButton.OnPress (lambda: PaperDoll.OpenPaperDollWindow(pc))
		if not Exportable:
			ColorButton.SetState (IE_GUI_BUTTON_DISABLED)

	ScriptButton = CustomizeWindow.GetControl (3)
	ScriptButton.SetText (17111)

	#This button does not exist in bg1 and pst, but theoretically we could create it here
	if not (GameCheck.IsBG1() or GameCheck.IsPST()):
		BiographyButton = CustomizeWindow.GetControl (9)
		BiographyButton.SetText (18003)
		BiographyButton.OnPress (OpenBiographyEditWindow)
		if not Exportable:
			BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	TextArea = CustomizeWindow.GetControl (5)
	TextArea.SetText (11327)

	CustomizeDoneButton = CustomizeWindow.GetControl (7)
	CustomizeDoneButton.SetText (11973)
	CustomizeDoneButton.SetState (IE_GUI_BUTTON_ENABLED)

	CancelButton = CustomizeWindow.GetControl (8)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	PortraitSelectButton.OnPress (OpenPortraitSelectWindow)
	SoundButton.OnPress (OpenSoundWindow)
	ScriptButton.OnPress (OpenScriptWindow)
	CustomizeDoneButton.OnPress (CustomizeWindow.Close)
	CancelButton.OnPress (CustomizeWindow.Close) # FIXME: this should revert changes I assume

	CustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def OpenPortraitSelectWindow ():
	global PortraitPictureButton

	SubCustomizeWindow = GemRB.LoadWindow (18)
	SubCustomizeWindow.AddAlias("SUB_WIN", 0)

	PortraitPictureButton = SubCustomizeWindow.GetControl (0)
	PortraitPictureButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitPictureButton.SetState (IE_GUI_BUTTON_LOCKED)

	PortraitLeftButton = SubCustomizeWindow.GetControl (1)
	PortraitLeftButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitLeftButton.OnPress (PortraitLeftPress)

	PortraitRightButton = SubCustomizeWindow.GetControl (2)
	PortraitRightButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitRightButton.OnPress (PortraitRightPress)

	PortraitDoneButton = SubCustomizeWindow.GetControl (3)
	PortraitDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitDoneButton.OnPress (PortraitDonePress)
	PortraitDoneButton.SetText (11973)
	PortraitDoneButton.MakeDefault()

	PortraitCancelButton = SubCustomizeWindow.GetControl (4)
	PortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCancelButton.OnPress (SubCustomizeWindow.Close)
	PortraitCancelButton.SetText (13727)
	PortraitCancelButton.MakeEscape()

	PortraitCustomButton = SubCustomizeWindow.GetControl (5)
	PortraitCustomButton.SetState (IE_GUI_BUTTON_ENABLED)
	PortraitCustomButton.OnPress (OpenCustomPortraitWindow)
	PortraitCustomButton.SetText (17545)

	# get players gender and portrait
	Pc = GemRB.GameGetSelectedPCSingle ()
	PcPortrait = GemRB.GetPlayerPortrait (Pc, 0)["ResRef"]

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
	GemRB.GetView("SUB_WIN", 0).Close()
	return

def PortraitLeftPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Previous () + PortraitNameSuffix, "NOPORTLG")

def PortraitRightPress ():
	global PortraitPictureButton

	PortraitPictureButton.SetPicture (Portrait.Next () + PortraitNameSuffix, "NOPORTLG")

def OpenCustomPortraitWindow ():
	global PortraitList1, PortraitList2
	global RowCount1, RowCount2

	Window = GemRB.LoadWindow (19)
	Window.AddAlias("SUB_WIN", 1)

	CustomPortraitDoneButton = Window.GetControl (10)
	CustomPortraitDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	CustomPortraitDoneButton.OnPress (CustomPortraitDonePress)
	CustomPortraitDoneButton.SetText (11973)
	CustomPortraitDoneButton.MakeDefault()

	CustomPortraitCancelButton = Window.GetControl (11)
	CustomPortraitCancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CustomPortraitCancelButton.OnPress (Window.Close)
	CustomPortraitCancelButton.SetText (13727)
	CustomPortraitCancelButton.MakeEscape()

	if not GameCheck.IsIWD1():
		SmallPortraitButton = Window.GetControl (1)
		SmallPortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
		LargePortraitButton = Window.GetControl (0)
		LargePortraitButton.SetFlags (IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)

	# Portrait List Large
	PortraitList1 = Window.GetControl (2)
	if GameCheck.IsIWD2():
		RowCount1 = len(PortraitList1.ListResources (CHR_PORTRAITS, 2))
	else:
		RowCount1 = len(PortraitList1.ListResources (CHR_PORTRAITS, 1))
	PortraitList1.OnSelect (LargeCustomPortrait)
	PortraitList1.SetVarAssoc ("Row1",RowCount1)

	# Portrait List Small
	PortraitList2 = Window.GetControl (3)
	RowCount2 = len(PortraitList2.ListResources (CHR_PORTRAITS, 0))
	PortraitList2.OnSelect (SmallCustomPortrait)
	PortraitList2.SetVarAssoc ("Row2",RowCount2)

	Window.ShowModal (MODAL_SHADOW_GRAY)
	return

def CustomPortraitDonePress ():
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.FillPlayerInfo (pc, PortraitList1.QueryText () , PortraitList2.QueryText ())
	
	#closing the generic portraits, because we just set a custom one
	subwin = GemRB.GetView("SUB_WIN", 0)
	if subwin:
		subwin.Close()
		
	GemRB.GetView("SUB_WIN", 1).Close()
	return

def LargeCustomPortrait ():
	Window = GemRB.GetView("SUB_WIN", 1)

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
	Window = GemRB.GetView("SUB_WIN", 1)

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

	pc = GemRB.GameGetSelectedPCSingle ()
	OldVoiceSet = GemRB.GetPlayerSound (pc)
	SubCustomizeWindow = GemRB.LoadWindow (20)
	SubCustomizeWindow.AddAlias("SUB_WIN", 0)

	VoiceList = SubCustomizeWindow.GetControl (5)
	Voices = VoiceList.ListResources (CHR_SOUNDS)

	# add "default" voice
	# bg1: mainm and mainf
	# bg2: last item in the list: female4/male005
	# iwds: n/a
	DefaultAdded = GUICommon.AddDefaultVoiceSet (VoiceList, Voices)

	# find the index of the current voice and preselect it
	if OldVoiceSet in Voices:
		VoiceList.SetVarAssoc ("Selected", Voices.index(OldVoiceSet) + int(DefaultAdded))
	else: # "default"
		VoiceList.SetVarAssoc ("Selected", 0)

	PlayButton = SubCustomizeWindow.GetControl (7)
	PlayButton.SetText (17318)

	TextArea = SubCustomizeWindow.GetControl (8)
	TextArea.SetText (11315)

	DoneButton = SubCustomizeWindow.GetControl (10)
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()

	CancelButton = SubCustomizeWindow.GetControl (11)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	PlayButton.OnPress (PlaySoundPressed)
	DoneButton.OnPress (DoneSoundWindow)
	CancelButton.OnPress (CloseSoundWindow)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def CloseSoundWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SetPlayerSound (pc, OldVoiceSet)
	GemRB.GetView("SUB_WIN", 0).Close()
	return

def DoneSoundWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	CharSound = VoiceList.QueryText ()
	CharSound = GUICommon.OverrideDefaultVoiceSet (Gender, CharSound)
	GemRB.SetPlayerSound (pc, CharSound)

	GemRB.GetView("SUB_WIN", 0).Close()
	return

def PlaySoundPressed():
	global SoundIndex, SoundSequence

	CharSound = VoiceList.QueryText ()
	SoundSeq = SoundSequence
	if CharSound == "default":
		SoundSeq = SoundSequence2
	CharSound = GUICommon.OverrideDefaultVoiceSet (Gender, CharSound)
	pc = GemRB.GameGetSelectedPCSingle ()

	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		GemRB.SetPlayerSound (pc, CharSound)
		VoiceSet = GemRB.GetPlayerSound (pc, 1)
	else:
		VoiceSet = CharSound
	tmp = SoundIndex
	while (not GemRB.HasResource (VoiceSet + SoundSeq[SoundIndex], RES_WAV)):
		NextSound()
		if SoundIndex == tmp:
			break
	else:
		NextSound()
	GemRB.PlaySound (VoiceSet + SoundSeq[SoundIndex], "CHARACT" + str(pc - 1), 0, 0, 5)
	return

def NextSound():
	global SoundIndex, SoundSequence
	SoundIndex += 1
	if SoundIndex >= len(SoundSequence):
		SoundIndex = 0
	return

def FindScriptFile(selected):
	script = ""
	for (filename, idx) in options:
		if idx == selected:
			script = filename
			break
	return script

options = {}
def OpenScriptWindow ():
	global ScriptTextArea, SelectedTextArea
	global options

	SubCustomizeWindow = GemRB.LoadWindow (11)
	SubCustomizeWindow.AddAlias("SUB_WIN", 0)

	ScriptTextArea = SubCustomizeWindow.GetControl (2)
	scripts = ScriptTextArea.ListResources (CHR_SCRIPTS)
	defaultCount = ScriptsTable.GetRowCount ()

	# (filename, idx in list of descriptions) indexing descriptions
	options = collections.OrderedDict()
	idx = 0
	# prepend the entry for none.bs manually, so we preserve the display order
	options[("none", idx)] = "None\n"
	for script in scripts:
		scriptindex = -1
		# can't use FindValue due to the need for lowercasing for case insensitive filesystems
		for i in range(defaultCount):
			name = ScriptsTable.GetRowName (i)
			GemRB.SetToken ("script", name)
			if name.lower() == script.lower():
				scriptindex = i
				break;

		idx = idx + 1
		if scriptindex == -1:
			# custom
			GemRB.SetToken ("script", script)
			options[(script, idx)] = GemRB.GetString (17167) + "\n"
		else:
			title = ScriptsTable.GetValue (scriptindex, 0, GTV_REF)
			if title:
				desc = ScriptsTable.GetValue (scriptindex, 1, GTV_REF)
				txt = title
				if desc:
					txt += " " + desc.rstrip()
				options[(script, idx)] = txt + "\n"*2
			else:
				# none
				idx = idx - 1

	ScriptTextArea.SetOptions (list(options.values()))

	pc = GemRB.GameGetSelectedPCSingle ()
	script = GemRB.GetPlayerScript (pc)
	if script == None:
		script = "None"

	scriptindex = 0
	for (filename, idx) in options:
		if filename == script:
			scriptindex = idx
			break

	SelectedTextArea = SubCustomizeWindow.GetControl (4)

	def UpdateScriptSelection(ta):
		name = FindScriptFile (ta.Value)
		SelectedTextArea.SetText (options[(name, ta.Value)])
		return
	
	ScriptTextArea.OnSelect (UpdateScriptSelection)
	ScriptTextArea.SetVarAssoc("Selected", scriptindex)

	DoneButton = SubCustomizeWindow.GetControl (5)
	DoneButton.SetText (11973)
	DoneButton.MakeDefault()

	CancelButton = SubCustomizeWindow.GetControl (6)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	DoneButton.OnPress (DoneScriptWindow)
	CancelButton.OnPress (SubCustomizeWindow.Close)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def DoneScriptWindow ():
	pc = GemRB.GameGetSelectedPCSingle ()
	selected = GemRB.GetVar ("Selected")
	script = FindScriptFile (selected)
	GemRB.SetPlayerScript (pc, script)
	GemRB.GetView("SUB_WIN", 0).Close()
	return

def RevertBiography(ta):
	global BioStrRef
	global RevertButton

	if GameCheck.IsIWD2():
		BioTable = GemRB.LoadTable ("bios")
		pc = GemRB.GameGetSelectedPCSingle ()
		ClassName = GUICommon.GetClassRowName (pc)
		BioStrRef = BioTable.GetValue (ClassName, "BIO")
	else:
		BioStrRef = 33347
	ta.SetText (BioStrRef)
	RevertButton.SetState (IE_GUI_BUTTON_DISABLED)
	return

def OpenBiographyEditWindow ():
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
		
	SubCustomizeWindow.AddAlias("SUB_WIN", 0)

	ClearButton = SubCustomizeWindow.GetControl (5)
	if GameCheck.IsBG2():
		ClearButton.SetText (34881)
	else:
		ClearButton.SetText (18622)

	DoneButton = SubCustomizeWindow.GetControl (1)
	DoneButton.SetText (11973)

	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		RevertButton = SubCustomizeWindow.GetControl (6)
	else:
		RevertButton = SubCustomizeWindow.GetControl (3)
	RevertButton.SetText (2240)
	if not Changed:
		RevertButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = SubCustomizeWindow.GetControl (2)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	TextArea = SubCustomizeWindow.ReplaceSubview(4, IE_GUI_TEXTAREA, "NORMAL")
	TextArea.SetFlags(IE_GUI_TEXTAREA_EDITABLE, OP_OR)
	TextArea.SetColor (ColorWhitish, TA_COLOR_NORMAL)
	TextArea.SetText (BioStrRef)
	TextArea.Focus ()

	ClearButton.OnPress (lambda: ClearBiography(TextArea))
	DoneButton.OnPress (lambda: DoneBiographyWindow(TextArea))
	RevertButton.OnPress (lambda: RevertBiography(TextArea))
	CancelButton.OnPress (SubCustomizeWindow.Close)

	SubCustomizeWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def ClearBiography(ta):
	global BioStrRef
	global RevertButton

	pc = GemRB.GameGetSelectedPCSingle ()
	#pc is 1 based
	BioStrRef = 62015+pc
	ta.SetText ("")
	ta.Focus()
	RevertButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

def DoneBiographyWindow (ta):
	global BioStrRef

	pc = GemRB.GameGetSelectedPCSingle ()
	if BioStrRef != 33347:
		GemRB.CreateString (BioStrRef, ta.QueryText())
	GemRB.SetPlayerString (pc, BioStrRefSlot, BioStrRef)
	GemRB.GetView("SUB_WIN", 0).Close()
	return

def OpenBiographyWindow ():
	BiographyWindow = GemRB.LoadWindow (12)

	TextArea = BiographyWindow.GetControl (0)
	pc = GemRB.GameGetSelectedPCSingle ()
	if GameCheck.IsBG1 () and GemRB.GetPlayerName (pc, 2) == 'none':
		TextArea.SetText (GetProtagonistBiography (pc))
	else:
		TextArea.SetText (GemRB.GetPlayerString (pc, BioStrRefSlot))

	# Done
	Button = BiographyWindow.GetControl (2)
	Button.SetText (11973)
	Button.OnPress (BiographyWindow.Close)

	BiographyWindow.ShowModal (MODAL_SHADOW_GRAY)
	return

def GetProtagonistBiography (pc):
	BioTable = GemRB.LoadTable ("bios")
	racestrings = [ 15895, 15891, 15892, 15890, 15893, 15894 ]

	ClassName = GUICommon.GetClassRowName (pc)
	bio = BioTable.GetValue (ClassName, "BIO", GTV_REF)
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
	TextArea.ListResources (CHR_EXPORTS)

	ExportDoneButton = ExportWindow.GetControl (4)
	ExportDoneButton.SetText (11973)
	ExportDoneButton.SetState (IE_GUI_BUTTON_DISABLED)

	CancelButton = ExportWindow.GetControl (5)
	CancelButton.SetText (13727)
	CancelButton.MakeEscape()

	NameField = ExportWindow.GetControl (6)

	ExportDoneButton.OnPress (ExportDonePress)
	CancelButton.OnPress (ExportCancelPress)
	NameField.OnChange (ExportEditChanged)
	ExportWindow.ShowModal (MODAL_SHADOW_GRAY)
	NameField.Focus()
	return

def ExportDonePress():
	#save file under name from EditControl
	pc = GemRB.GameGetSelectedPCSingle ()
	GemRB.SaveCharacter (pc, NameField.QueryText ())
	if ExportWindow:
		ExportWindow.Close ()
	return

def ExportCancelPress():
	global ExportWindow
	if ExportWindow:
		ExportWindow.Close ()
		ExportWindow = None
	return

def ExportEditChanged():
	ExportFileName = NameField.QueryText ()
	if ExportFileName == "":
		ExportDoneButton.SetState (IE_GUI_BUTTON_DISABLED)
	else:
		ExportDoneButton.SetState (IE_GUI_BUTTON_ENABLED)
	return

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
# common character generation display code
import GemRB
from ie_stats import *
from GUIDefines import *
import GameCheck
import GUICommon
import CommonTables
import Spellbook
from ie_restype import RES_BMP

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def PositionCharGenWin(window, yOffset = 0, xOffset = 0):
	global CharGenWindow
	
	CGFrame = CharGenWindow.GetFrame()
	WFrame = window.GetFrame()
	window.SetPos(CGFrame['x'] + xOffset, yOffset + CGFrame['y'] + (CGFrame['h'] - WFrame['h']))


def DisplayOverview(step):
	"""Sets up the primary character generation window."""

	global CharGenWindow, TextAreaControl, PortraitName

	CharGenWindow = GemRB.LoadWindow (0, "GUICG")
	GemRB.SetVar ("Step", step)

	###
	# Buttons
	###
	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	if PortraitName != None:
		if GemRB.HasResource (PortraitName, RES_BMP, 1) or GemRB.HasResource ("NOPORTMD", RES_BMP, 1):
			PortraitButton.SetPicture (PortraitName, "NOPORTMD")
	PortraitButton.SetState (IE_GUI_BUTTON_LOCKED)

	GenderButton = CharGenWindow.GetControl (0)
	GenderButton.SetText (11956)
	SetButtonStateFromStep ("GenderButton", GenderButton, step)

	RaceButton = CharGenWindow.GetControl (1)
	RaceButton.SetText (11957)
	SetButtonStateFromStep ("RaceButton", RaceButton, step)

	ClassButton = CharGenWindow.GetControl (2)
	ClassButton.SetText (11959)
	SetButtonStateFromStep ("ClassButton", ClassButton, step)

	AlignmentButton = CharGenWindow.GetControl (3)
	AlignmentButton.SetText (11958)
	SetButtonStateFromStep ("AlignmentButton", AlignmentButton, step)

	AbilitiesButton = CharGenWindow.GetControl (4)
	AbilitiesButton.SetText (11960)
	SetButtonStateFromStep ("AbilitiesButton", AbilitiesButton, step)

	SkillButton = CharGenWindow.GetControl (5)
	SkillButton.SetText (17372)
	SetButtonStateFromStep ("SkillButton", SkillButton, step)

	AppearanceButton = CharGenWindow.GetControl (6)
	AppearanceButton.SetText (11961)
	SetButtonStateFromStep ("AppearanceButton", AppearanceButton, step)

	NameButton = CharGenWindow.GetControl (7)
	NameButton.SetText (11963)
	SetButtonStateFromStep ("NameButton", NameButton, step)

	BackButton = CharGenWindow.GetControl (11)
	BackButton.SetText (15416)
	BackButton.SetState (IE_GUI_BUTTON_ENABLED)
	if GemRB.GetVar ("ImportedChar"):
		BackButton.SetState (IE_GUI_BUTTON_DISABLED)
	BackButton.OnPress (BackPress)
	BackButton.MakeEscape()

	AcceptButton = CharGenWindow.GetControl (8)
	playmode = GemRB.GetVar ("PlayMode")
	if playmode is not None:
		AcceptButton.SetText (11962)
	else:
		AcceptButton.SetText (13956)
	SetButtonStateFromStep ("AcceptButton", AcceptButton, step)
	#AcceptButton.MakeDefault()
	AcceptButton.OnPress (NextPress)

	# now automatically ignored and added instead
	#ScrollBar = CharGenWindow.GetControl (10)
	#ScrollBar.SetDefaultScrollBar ()

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetText (13955)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportButton.OnPress (ImportPress)

	CancelButton = CharGenWindow.GetControl (15)
	if step == 1:
		CancelButton.SetText (13727) # Cancel
	else:
		CancelButton.SetText (8159) # Start over
		CancelButton.SetDisabled(False)
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CancelButton.OnPress (CancelPress)

	BiographyButton = CharGenWindow.GetControl (16)
	BiographyButton.SetText (18003)
	BiographyButton.OnPress (BiographyPress)
	if step == 9:
		BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	###
	# Stat overview
	###
	AbilityTable = GemRB.LoadTable ("ability")

	MyChar = GemRB.GetVar ("Slot")
	TextAreaControl= CharGenWindow.GetControl (9)
	TextAreaControl.SetText ("")

	for part in range(1, step+1):
		if part == 1:
			if step == 1:
				TextAreaControl.SetText (GemRB.GetString(16575))
			elif step == 9:
				TextAreaControl.SetText ("")
		elif part == 2:
			if step == 9:
				TextAreaControl.Append (GemRB.GetString(1047) + ": " + GemRB.GetToken ("CHARNAME") + "\n")
				
			if GemRB.GetPlayerStat (MyChar, IE_SEX) == 1:
				gender = GemRB.GetString (1050)
			else:
				gender = GemRB.GetString (1051)
			TextAreaControl.Append (GemRB.GetString(12135) + ": " + gender + "\n")
		elif part == 3:
			stat = GemRB.GetPlayerStat(MyChar, IE_RACE)
			v = CommonTables.Races.FindValue (3, stat)
			TextAreaControl.Append (GemRB.GetString (1048) + ": " + CommonTables.Races.GetValue (v, 2, GTV_REF) + "\n")
		elif part == 4:
			ClassTitle = GUICommon.GetActorClassTitle (MyChar)
			TextAreaControl.Append (GemRB.GetString(12136) + ": " + ClassTitle + "\n")
		elif part == 5:
			stat = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
			v = CommonTables.Aligns.FindValue (3, stat)
			TextAreaControl.Append (GemRB.GetString(1049) + ": " + CommonTables.Aligns.GetValue (v, 2, GTV_REF) + "\n")
		elif part == 6:
			TextAreaControl.Append ("\n")
			ClassName = GUICommon.GetClassRowName (MyChar)
			strextra = GemRB.GetPlayerStat (MyChar, IE_STREXTRA)
			for i in range(6):
				v = AbilityTable.GetValue (i, 2, GTV_REF)
				StatID = AbilityTable.GetValue (i, 3)
				stat = GemRB.GetPlayerStat (MyChar, StatID)
				if (i == 0) and (strextra > 0) and (stat==18):
					TextAreaControl.Append (v + ": " + str(stat) + "/" + str(strextra) + "\n")
				else:
					TextAreaControl.Append (v + ": " + str(stat) + "\n")
		elif part == 7:
			# thieving and other skills
			info = ""
			SkillTable = GemRB.LoadTable ("skills")
			RangerSkills = CommonTables.ClassSkills.GetValue (ClassName, "RANGERSKILL")
			BardSkills = CommonTables.ClassSkills.GetValue (ClassName, "BARDSKILL")
			KitName = GUICommon.GetKitIndex (MyChar)
			if KitName == 0:
				KitName = ClassName
			else:
				KitName = CommonTables.KitList.GetValue (KitName, 0)

			if SkillTable.GetValue ("RATE", KitName) != -1:
				for skill in range(SkillTable.GetRowCount () - 2):
					name = SkillTable.GetValue (skill+2, 1, GTV_REF)
					available = SkillTable.GetValue (SkillTable.GetRowName (skill+2), KitName)
					statID = SkillTable.GetValue (skill+2, 2)
					value = GemRB.GetPlayerStat (MyChar, statID, 1)
					if value >= 0 and available != -1:
						info += name + ": " + str(value) + "\n"
			elif BardSkills != "*" or RangerSkills != "*":
				for skill in range(SkillTable.GetRowCount () - 2):
					name = SkillTable.GetValue (skill+2, 1, GTV_REF)
					StatID = SkillTable.GetValue (skill+2, 2)
					value = GemRB.GetPlayerStat (MyChar, StatID, 1)
					if value > 0:
						info += name + ": " + str(value) + "\n"
			if info != "":
				TextAreaControl.Append ("\n" + GemRB.GetString(8442) + "\n" + info)

			# arcane spells
			info = Spellbook.GetKnownSpellsDescription (MyChar, IE_SPELL_TYPE_WIZARD)
			if info != "":
				TextAreaControl.Append ("\n" + GemRB.GetString(11027) + "\n" + info)

			# divine spells
			info = Spellbook.GetKnownSpellsDescription (MyChar, IE_SPELL_TYPE_PRIEST)
			if info != "":
				TextAreaControl.Append ("\n" + GemRB.GetString(11028) + "\n" + info)

			# racial enemy
			info = ""
			Race = GemRB.GetVar ("HatedRace")
			if Race:
				HateRaceTable = GemRB.LoadTable ("HATERACE")
				Row = HateRaceTable.FindValue (1, Race)
				info = HateRaceTable.GetValue (Row, 0, GTV_REF) + "\n"
				if info != "":
					TextAreaControl.Append ("\n" + GemRB.GetString(15982) + "\n" + info)

			# weapon proficiencies
			TextAreaControl.Append ("\n")
			TextAreaControl.Append (9466)
			TextAreaControl.Append ("\n")
			TmpTable=GemRB.LoadTable ("weapprof")
			ProfCount = TmpTable.GetRowCount ()
			#bg2 weapprof.2da contains the bg1 proficiencies too, skipping those
			for i in range(ProfCount-8):
				# 4294967296 overflows to -1 on some arches, so we use a smaller invalid strref
				strref = TmpTable.GetValue (i+8, 1)
				if strref == -1 or strref > 500000:
					continue
				Weapon = GemRB.GetString (strref)
				StatID = TmpTable.GetValue (i+8, 0)
				Value = GemRB.GetPlayerStat (MyChar, StatID )
				if Value:
					pluses = " "
					for plus in range(0, Value):
						pluses += "+"
					TextAreaControl.Append (Weapon + pluses + "\n")

		elif part == 8:
			break

	CharGenWindow.Focus()
	return

def SetButtonStateFromStep (buttonName, button, step):
	"""Updates selectable buttons based upon current step."""

	global CharGenWindow

	state = IE_GUI_BUTTON_DISABLED
	if buttonName == "GenderButton":
		if step == 1:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "RaceButton":
		if step == 2:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "ClassButton":
		if step == 3:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AlignmentButton":
		if step == 4:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AbilitiesButton":
		if step == 5:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "SkillButton":
		if step == 6:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AppearanceButton":
		if step == 7:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "NameButton":
		if step == 8:
			state = IE_GUI_BUTTON_ENABLED
	elif buttonName == "AcceptButton":
		if step == 9:
			state = IE_GUI_BUTTON_ENABLED
		elif GameCheck.IsBG2Demo ():
			state = IE_GUI_BUTTON_LOCKED
	button.SetState (state)

	if state == IE_GUI_BUTTON_ENABLED:
		button.Focus()
		button.OnPress (NextPress)
	return

def CancelPress():
	"""Revert back to the first step; if there, free the actor."""

	step = GemRB.GetVar ("Step")
	if step == 1:
		global CharGenWindow
		if CharGenWindow:
			CharGenWindow.Close ()
		#free up the slot before exiting
		MyChar = GemRB.GetVar ("Slot")
		GemRB.CreatePlayer ("", MyChar | 0x8000 )
	else:
		GemRB.SetNextScript ("CharGen")
		GemRB.SetToken ("LargePortrait", "")
		GemRB.SetToken ("SmallPortrait", "")
	return

def ImportPress():
	"""Opens the character import window."""

	step = GemRB.GetVar ("Step")
	# TODO: check why this is handled differently
	if step == 1:
		GemRB.SetNextScript("GUICG24")
	else:
		GemRB.SetToken ("NextScript", "CharGen9")
		GemRB.SetNextScript ("ImportFile") #import
	return

def BackPress():
	"""Moves to the previous step."""

	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Close ()

	step = GemRB.GetVar ("Step")
	if step == 2:
		GemRB.SetNextScript ("CharGen")
	elif step != 1:
		GemRB.SetNextScript ("CharGen" + str(step-1))
	return

def NextPress():
	"""Moves to the next step."""

	step = GemRB.GetVar ("Step")
	if step == 1:
		CharGenWindow.GetControl(15).SetDisabled(True)
		GemRB.SetNextScript ("GUICG1")
	elif step == 2:
		GemRB.SetNextScript ("GUICG8")
	elif step == 6:
		GemRB.SetNextScript ("GUICG15")
	elif step == 7:
		GemRB.SetNextScript ("GUICG13")
	elif step == 8:
		GemRB.SetNextScript ("GUICG5")
	elif step == 9:
		GemRB.SetNextScript ("CharGenEnd")
	else: # 3, 4, 5
		GemRB.SetNextScript ("GUICG" + str(step-1))
	return

def BiographyPress():
	"""Opens the biography window."""
	GemRB.SetNextScript("GUICG23") #biography
	return

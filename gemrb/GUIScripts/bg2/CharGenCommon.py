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
import GUICommon
import CommonTables
from ie_restype import RES_BMP

CharGenWindow = 0
TextAreaControl = 0
PortraitName = ""

def DisplayOverview(step):
	"""Sets up the primary character generation window."""

	global CharGenWindow, TextAreaControl, PortraitName

	GemRB.LoadWindowPack ("GUICG", 640, 480)
	CharGenWindow = GemRB.LoadWindow (0)
	CharGenWindow.SetFrame ()
	GemRB.SetVar ("Step", step)

	###
	# Buttons
	###
	PortraitButton = CharGenWindow.GetControl (12)
	PortraitButton.SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_NO_IMAGE,OP_SET)
	PortraitName = GemRB.GetToken ("LargePortrait")
	if PortraitName != "":
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
	BackButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BackPress)
	BackButton.SetFlags (IE_GUI_BUTTON_CANCEL, OP_OR)

	AcceptButton = CharGenWindow.GetControl (8)
	playmode = GemRB.GetVar ("PlayMode")
	if playmode>=0:
		AcceptButton.SetText (11962)
	else:
		AcceptButton.SetText (13956)
	SetButtonStateFromStep ("AcceptButton", AcceptButton, step)
	#AcceptButton.SetFlags(IE_GUI_BUTTON_DEFAULT,OP_OR)
	AcceptButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, NextPress)

	ScrollBar = CharGenWindow.GetControl (10)
	ScrollBar.SetDefaultScrollBar ()

	ImportButton = CharGenWindow.GetControl (13)
	ImportButton.SetText (13955)
	ImportButton.SetState (IE_GUI_BUTTON_ENABLED)
	ImportButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, ImportPress)

	CancelButton = CharGenWindow.GetControl (15)
	if step == 1:
		CancelButton.SetText (13727) # Cancel
	else:
		CancelButton.SetText (8159) # Start over
	CancelButton.SetState (IE_GUI_BUTTON_ENABLED)
	CancelButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, CancelPress)

	BiographyButton = CharGenWindow.GetControl (16)
	BiographyButton.SetText (18003)
	BiographyButton.SetEvent (IE_GUI_BUTTON_ON_PRESS, BiographyPress)
	if step == 9:
		BiographyButton.SetState (IE_GUI_BUTTON_ENABLED)
	else:
		BiographyButton.SetState (IE_GUI_BUTTON_DISABLED)

	###
	# Stat overview
	###
	AbilityTable = GemRB.LoadTable ("ability")

	MyChar = GemRB.GetVar ("Slot")

	for part in range(1, step+1):
		if part == 1:
			TextAreaControl= CharGenWindow.GetControl (9)
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
			TextAreaControl.Append (GemRB.GetString(1048) + ": " + GemRB.GetString( CommonTables.Races.GetValue (v,2)) + "\n")
		elif part == 4:
			ClassTitle = GUICommon.GetActorClassTitle (MyChar)
			TextAreaControl.Append (GemRB.GetString(12136) + ": " + ClassTitle + "\n")
		elif part == 5:
			stat = GemRB.GetPlayerStat (MyChar, IE_ALIGNMENT)
			v = CommonTables.Aligns.FindValue (3, stat)
			TextAreaControl.Append (GemRB.GetString(1049) + ": " + GemRB.GetString(CommonTables.Aligns.GetValue (v,2)) + "\n")
		elif part == 6:
			TextAreaControl.Append ("\n")
			ClassName = GUICommon.GetClassRowName (MyChar)
			hasextra = CommonTables.Classes.GetValue (ClassName, "SAVE") == "SAVEWAR"
			strextra = GemRB.GetPlayerStat (MyChar, IE_STREXTRA)
			for i in range(6):
				v = AbilityTable.GetValue (i, 2)
				StatID = AbilityTable.GetValue (i, 3)
				stat = GemRB.GetPlayerStat (MyChar, StatID)
				if (i == 0) and hasextra and (stat==18):
					TextAreaControl.Append (GemRB.GetString(v) + ": " + str(stat) + "/" + str(strextra) + "\n")
				else:
					TextAreaControl.Append (GemRB.GetString(v) + ": " + str(stat) + "\n")
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
					name = GemRB.GetString (SkillTable.GetValue (skill+2, 1))
					available = SkillTable.GetValue (SkillTable.GetRowName (skill+2), KitName)
					statID = SkillTable.GetValue (skill+2, 2)
					value = GemRB.GetPlayerStat (MyChar, statID, 1)
					if value >= 0 and available != -1:
						info += name + ": " + str(value) + "\n"
			elif BardSkills != "*" or RangerSkills != "*":
				for skill in range(SkillTable.GetRowCount () - 2):
					name = GemRB.GetString (SkillTable.GetValue (skill+2, 1))
					StatID = SkillTable.GetValue (skill+2, 2)
					value = GemRB.GetPlayerStat (MyChar, StatID, 1)
					if value > 0:
						info += name + ": " + str(value) + "\n"
			if info != "":
				TextAreaControl.Append (GemRB.GetString(8442) + info)

			# arcane spells
			info = ""
			for level in range(0, 9):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_WIZARD, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_WIZARD, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
					info += GemRB.GetString (Spell)
			if info != "":
				TextAreaControl.Append ( GemRB.GetString(11027) + info)

			# divine spells
			info = ""
			for level in range(0, 7):
				for j in range(0, GemRB.GetKnownSpellsCount (MyChar, IE_SPELL_TYPE_PRIEST, level) ):
					Spell = GemRB.GetKnownSpell (MyChar, IE_SPELL_TYPE_PRIEST, level, j)
					Spell = GemRB.GetSpell (Spell['SpellResRef'], 1)['SpellName']
					info += GemRB.GetString (Spell)
			if info != "":
				TextAreaControl.Append (GemRB.GetString(11028) + info)

			# racial enemy
			info = ""
			Race = GemRB.GetVar ("HatedRace")
			if Race:
				HateRaceTable = GemRB.LoadTable ("HATERACE")
				Row = HateRaceTable.FindValue (1, Race)
				info = GemRB.GetString (HateRaceTable.GetValue(Row, 0))
				if info != "":
					TextAreaControl.Append (GemRB.GetString(15982) + info)

			# weapon proficiencies
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
					TextAreaControl.Append (Weapon + pluses)

		elif part == 8:
			break

	CharGenWindow.SetVisible (WINDOW_VISIBLE)
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
	button.SetState (state)

	if state == IE_GUI_BUTTON_ENABLED:
		button.SetFlags (IE_GUI_BUTTON_DEFAULT, OP_OR)
		button.SetEvent (IE_GUI_BUTTON_ON_PRESS, NextPress)
	return

def CancelPress():
	"""Revert back to the first step; if there, free the actor."""

	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
		#free up the slot before exiting
		MyChar = GemRB.GetVar ("Slot")
		GemRB.CreatePlayer ("", MyChar | 0x8000 )
		GemRB.SetNextScript ("Start")
	else:
		GemRB.SetNextScript ("CharGen")
		GemRB.SetToken ("LargePortrait", "")
		GemRB.SetToken ("SmallPortrait", "")
	return

def ImportPress():
	"""Opens the character import window."""

	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

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
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
		GemRB.SetNextScript ("Start")
	elif step == 2:
		GemRB.SetNextScript ("CharGen")
	else:
		GemRB.SetNextScript ("CharGen" + str(step-1))
	return

def NextPress():
	"""Moves to the next step."""

	global CharGenWindow
	if CharGenWindow:
		CharGenWindow.Unload ()

	step = GemRB.GetVar ("Step")
	if step == 1:
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
	if CharGenWindow:
		CharGenWindow.Unload()
	GemRB.SetNextScript("GUICG23") #biography
	return

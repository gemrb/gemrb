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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
#
# GUICommon.py - common functions for GUIScripts of all game types

import GemRB
import GameCheck
import GUIClasses
import collections
import CommonTables
import Spellbook
from ie_restype import RES_CHU, RES_2DA, RES_BAM, RES_WAV
from ie_spells import LS_MEMO
from GUIDefines import *
from ie_stats import *

CommonTables.Load ()

def GetWindowPack():
	height = GemRB.GetSystemVariable (SV_HEIGHT)

	if GameCheck.IsPST ():
		default = "GUIWORLD"
	else:
		default = "GUIW"

	# select this based on height
	# we do this because:
	# 1. windows are never the entire width,
	#    but the can be the entire height
	# 2. the originals were for 4:3 screens,
	#    but modern screens are usually a wider aspect
	# 3. not all games have all the window packs
	# ... but the widescreen mod only modifies guiw08 or guiw10, not touching others
	# luckily internally its guiw08 is in the end produced as a copy of guiw10
	if GameCheck.HasWideScreenMod ():
		if GameCheck.IsBG1 () or GameCheck.IsPST ():
			return default
		return "GUIW10"
	else:
		if height >= 960 and GemRB.HasResource ("GUIW12", RES_CHU, 1):
			return "GUIW12"
		elif height >= 768 and GemRB.HasResource ("GUIW10", RES_CHU, 1):
			return "GUIW10"
		elif height >= 600 and GemRB.HasResource ("GUIW08", RES_CHU, 1):
			return "GUIW08"

	# fallback to the smallest resolution
	return default

def LocationPressed ():
	AreaInfo = GemRB.GetAreaInfo()
	TMessageTA = GemRB.GetView("MsgSys", 0)
	if TMessageTA:
		message = "[color=ff0000]Mouse:[/color] x={0}, y={1}\n[color=ff0000]Area:[/color] {2}\n"
		message = message.format(AreaInfo["PositionX"], AreaInfo["PositionY"], AreaInfo["CurrentArea"])
		TMessageTA.Append(message)
	else:
		print("%s [%d.%d]\n" % (AreaInfo["CurrentArea"], AreaInfo["PositionX"], AreaInfo["PositionY"]))

	return

def OpenFloatMenuWindow (p):
	if GameCheck.IsPST():
		import FloatMenuWindow
		FloatMenuWindow.OpenFloatMenuWindow(p['x'], p['y'])
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

def GetActorPaperDoll (actor):
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "LEVEL%d" %(level+1)
	doll = CommonTables.Pdolls.GetValue (row, which)
	if doll == "*":
		# guess a name
		import GUICommonWindows
		doll = GUICommonWindows.GetActorPaperDoll (actor) + "INV"
		if not GemRB.HasResource (doll, RES_BAM):
			print("GetActorPaperDoll: Missing paper doll for animation", row, which, doll)
	return doll

def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)

def GetAbilityBonus (Actor, Stat):
	Ability = GemRB.GetPlayerStat (Actor, Stat)
	return Ability//2-5

def SetColorStat (Actor, Stat, Value):
	t = Value & 0xFF
	t |= t << 8
	t |= t << 16
	GemRB.SetPlayerStat (Actor, Stat, t)
	return

def CheckStat100 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,100, Diff)
	if mystat>=goal:
		return True
	return False

def CheckStat20 (Actor, Stat, Diff):
	mystat = GemRB.GetPlayerStat (Actor, Stat)
	goal = GemRB.Roll (1,20, Diff)
	if mystat>=goal:
		return True
	return False

def GetGUISpellButtonCount ():
	if GameCheck.HasHOW() or GameCheck.IsBG2():
		return 24
	else:
		return 20

def SetGamedaysAndHourToken ():
	currentTime = GemRB.GetGameTime()
	days = currentTime // 7200
	hours = (currentTime % 7200) // 300
	GemRB.SetToken ('GAMEDAY', str (days))
	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

def Gain(infostr, ability):
	GemRB.SetToken ('SPECIALABILITYNAME', GemRB.GetString(int(ability) ) )
	GemRB.DisplayString (infostr, ColorWhite) # FIXME: what color should this really be

# chargen version of AddClassAbilities
def ResolveClassAbilities (pc, ClassName):
	# apply class/kit abilities
	IsMulti = IsMultiClassed (pc, 1)
	Levels = [GemRB.GetPlayerStat (pc, IE_LEVEL), GemRB.GetPlayerStat (pc, IE_LEVEL2), \
			GemRB.GetPlayerStat (pc, IE_LEVEL3)]
	KitIndex = GetKitIndex (pc)
	if IsMulti[0]>1:
		#get the class abilites for each class
		for i in range (IsMulti[0]):
			TmpClassName = GetClassRowName (IsMulti[i+1], "class")
			ABTable = CommonTables.ClassSkills.GetValue (TmpClassName, "ABILITIES")
			if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
				AddClassAbilities (pc, ABTable, Levels[i], Levels[i])
	else:
		if KitIndex:
			ABTable = CommonTables.KitList.GetValue (str(KitIndex), "ABILITIES")
		else:
			ABTable = CommonTables.ClassSkills.GetValue (ClassName, "ABILITIES")
		if ABTable != "*" and GemRB.HasResource (ABTable, RES_2DA, 1):
			AddClassAbilities (pc, ABTable, Levels[0], Levels[0])

# Adds class/kit abilities
def AddClassAbilities (pc, table, Level=1, LevelDiff=1, align=-1):
	TmpTable = GemRB.LoadTable (table)

	# gotta stay positive
	if Level-LevelDiff < 0:
		return

	# we're doing alignment additions
	if align == -1:
		iMin = 0
		iMax = TmpTable.GetRowCount ()
	else:
		# alignment is expected to be the row required
		iMin = align
		iMax = align+1

	# make sure we don't go out too far
	jMin = Level-LevelDiff
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(iMin, iMax):
		# apply each spell from each new class
		for j in range (jMin, jMax):
			ab = TmpTable.GetValue (i, j, GTV_STR)
			AddClassAbility(pc, ab)

def AddClassAbility (pc, ab):
	if not ab or ab == "****":
		return

	# seems all SPINs act like GA_*
	if ab[:4] == "SPIN":
		ab = "GA_" + ab

	# apply spell (AP_) or gain spell (GA_)
	if ab[:3] == "AP_":
		GemRB.ApplySpell (pc, ab[3:])
	elif ab[:3] == "GA_":
		Spellbook.LearnSpell (pc, ab[3:], IE_SPELL_TYPE_INNATE, 0, 1, LS_MEMO)
	elif ab[:3] == "FS_":
		Gain(26320, ab[3:])
	elif ab[:3] == "FA_":
		Gain(10514, ab[3:])
	else:
		GemRB.Log (LOG_ERROR, "AddClassAbilities", "Unknown class ability (type): " + ab)

def MakeSpellCount (pc, spell, count):
	have = GemRB.CountSpells (pc, spell, 1)
	if count<=have:
		return
	# only used for innates, which are all level 1
	Spellbook.LearnSpell (pc, spell, IE_IWD2_SPELL_INNATE, 0, count-have, LS_MEMO)
	return

# remove all class abilities up to the given level
# for dual-classing mainly
def RemoveClassAbilities (pc, table, Level):
	TmpTable = GemRB.LoadTable (table)

	# gotta stay positive
	if Level < 0:
		return

	# make sure we don't go out too far
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(TmpTable.GetRowCount ()):
		for j in range (jMax):
			ab = TmpTable.GetValue (i, j, GTV_STR)
			RemoveClassAbility (pc, ab)

def RemoveClassAbility (pc, ab):
	if not ab or ab == "****":
		return

	# get the index
	SpellIndex = Spellbook.HasSpell (pc, IE_SPELL_TYPE_INNATE, 0, ab[3:])

	# seems all SPINs act like GA_*
	if ab[:4] == "SPIN":
		ab = "GA_" + ab

	# apply spell (AP_) or gain spell (GA_)?
	if ab[:3] == "AP_":
		GemRB.RemoveEffects (pc, ab[3:])
	elif ab[:3] == "GA_":
		if SpellIndex >= 0:
			# TODO: get the correct counts to avoid removing an innate ability
			# given by more than one thing?
			# RemoveSpell will unmemorize them all too
			GemRB.RemoveSpell (pc, IE_SPELL_TYPE_INNATE, 0, SpellIndex)
	elif ab[:3] != "FA_" and ab[:3] != "FS_":
		GemRB.Log (LOG_ERROR, "RemoveClassAbilities", "Unknown class ability (type): " + ab)

# PST uses a button, IWD2 two types, the rest are the same with two labels
def SetEncumbranceLabels (Window, ControlID, Control2ID, pc):
	"""Displays the encumbrance as a ratio of current to maximum."""

	# encumbrance
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)
	max_encumb = GemRB.GetMaxEncumbrance (pc)

	Control = Window.GetControl (ControlID)
	if GameCheck.IsPST():
		# FIXME: there should be a space before LB symbol (':') - but there is no frame for it and our doesn't cut it
		Control.SetText (str (encumbrance) + ":\n\n" + str (max_encumb) + ":")
	elif GameCheck.IsIWD2() and not Control2ID:
		Control.SetText (str (encumbrance) + "/" + str(max_encumb) + GemRB.GetString(39537))
	else:
		Control.SetText (str (encumbrance) + ":")
		if not Control2ID: # shouldn't happen
			print("Missing second control parameter to SetEncumbranceLabels!")
			return
		Control2 = Window.GetControl (Control2ID)
		Control2.SetText (str (max_encumb) + ":")

	ratio = encumbrance / max_encumb
	if GameCheck.IsIWD2 () or GameCheck.IsPST ():
		if ratio > 1.0:
			Control.SetColor ({'r' : 255, 'g' : 0, 'b' : 0})
		elif ratio > 0.8:
			Control.SetColor ({'r' : 255, 'g' : 255, 'b' : 0})
		else:
			Control.SetColor ({'r' : 255, 'g' : 255, 'b' : 255})

		if Control2ID:
			Control2.SetColor ({'r' : 255, 'g' : 0, 'b' : 0})

	else:
		if ratio > 1.0:
			Control.SetFont ("NUMBER3");
		elif ratio > 0.8:
			Control.SetFont ("NUMBER2");
		else:
			Control.SetFont ("NUMBER");

	return

def GetActorClassTitle (actor):
	"""Returns the string representation of the actors class."""

	ClassTitle = GemRB.GetPlayerStat (actor, IE_TITLE1)
	if ClassTitle != 0:
		ClassTitle = GemRB.GetString (ClassTitle)
		return ClassTitle

	ClassName = GetClassRowName (actor)
	KitIndex = GetKitIndex (actor)
	Multi = HasMultiClassBits (actor)
	Dual = IsDualClassed (actor, 1)
	MCFlags = GemRB.GetPlayerStat (actor, IE_MC_FLAGS)

	if Multi and Dual[0] == 0: # true multi class
		ClassTitle = CommonTables.Classes.GetValue (ClassName, "CAP_REF", GTV_REF)
		return ClassTitle

	if Dual[0]: # dual class
		# first (previous) kit or class of the dual class
		if Dual[0] == 1:
			ClassTitle = CommonTables.KitList.GetValue (Dual[1], 2, GTV_REF)
		else:
			ClassTitle = CommonTables.Classes.GetValue (GetClassRowName(Dual[1], "index"), "CAP_REF", GTV_REF)
		ClassTitle += " / "
		if Dual[0] == 3:
			ClassTitle += CommonTables.KitList.GetValue (Dual[2], 2, GTV_REF)
		else:
			ClassTitle += CommonTables.Classes.GetValue (GetClassRowName(Dual[2], "index"), "CAP_REF", GTV_REF)
	elif MCFlags & (MC_FALLEN_PALADIN | MC_FALLEN_RANGER): # fallen
		ClassTitle = 10369
		if MCFlags & MC_FALLEN_PALADIN:
			ClassTitle = 10371
		ClassTitle = GemRB.GetString (ClassTitle)
	elif KitIndex: # ordinary kit
		ClassTitle = CommonTables.KitList.GetValue (KitIndex, 2, GTV_REF)
	else: # ordinary class
		ClassTitle = CommonTables.Classes.GetValue (ClassName, "CAP_REF", GTV_REF)

	return ClassTitle

def GetKitIndex (actor):
	"""Return the index of the actors kit from KITLIST.2da.

	Returns 0 if the class is not kitted."""

	Kit = GemRB.GetPlayerStat (actor, IE_KIT)
	KitIndex = 0

	if Kit & 0xc000 == 0x4000:
		KitIndex = Kit & 0xfff

	# carefully looking for kit by the usability flag
	# since the barbarian kit id clashes with the no-kit value
	if KitIndex == 0 and Kit != 0x4000:
		KitIndex = CommonTables.KitList.FindValue (6, Kit)
		if KitIndex is None:
			KitIndex = 0

	return KitIndex

# fetches the rowname of the passed actor's (base) class from classes.2da
# NOTE: only the "index" method is iwd2-ready, since you can have multiple classes and kits
def GetClassRowName(value, which=-1):
	if which == "index":
		ClassIndex = value
		# if barbarians cause problems, repeat the lookup again here
	else:
		if which == -1:
			Class = GemRB.GetPlayerStat (value, IE_CLASS)
		elif which == "class":
			Class = value
		else:
			raise RuntimeError("Bad type parameter for GetClassRowName: " + str(which))
		ClassIndex = CommonTables.Classes.FindValue ("ID", Class)
	if ClassIndex is None:
		return ""
	ClassRowName = CommonTables.Classes.GetRowName (ClassIndex)
	return ClassRowName

# checks the classes.2da table if the class is multiclass/dualclass capable (bits define the class combination)
def HasMultiClassBits(actor):
	MultiBits = CommonTables.Classes.GetValue (GetClassRowName(actor), "MULTI")

	# we have no entries for npc creature classes, so treat them as single-classed
	if MultiBits == "*":
		MultiBits = 0

	return MultiBits

def IsDualClassed(actor, verbose):
	"""Returns an array containing the dual class information.

	Return[0] is 0 if not dualclassed, 1 if the old class is a kit, 3 if the new class is a kit, 2 otherwise.
	Return[1] contains either the kit or class index of the old class.
	Return[2] contains the class index of the new class.
	If verbose is false, only Return[0] contains useable data."""

	Multi = HasMultiClassBits (actor)
	if Multi == 0: # also catches iwd2
		return (0, -1, -1)

	DualedFrom = GemRB.GetPlayerStat (actor, IE_MC_FLAGS) & MC_WAS_ANY_CLASS
	if DualedFrom == 0:
		return (0, -1, -1)

	if not verbose:
		return (1, -1, -1)

	KitIndex = GetKitIndex (actor)
	if KitIndex:
		KittedClass = CommonTables.KitList.GetValue (KitIndex, 7)
		KittedClassIndex = CommonTables.Classes.FindValue ("ID", KittedClass)
	else:
		KittedClassIndex = 0

	# first (previous) class of the dual class
	FirstClassIndex = CommonTables.Classes.FindValue ("MC_WAS_ID", DualedFrom)

	# use the first class of the multiclass bunch that isn't the same as the first class
	for i in range (1,16):
		Mask = 1 << (i - 1)
		if Multi & Mask:
			ClassIndex = CommonTables.Classes.FindValue ("ID", i)
			if ClassIndex == FirstClassIndex:
				continue
			SecondClassIndex = ClassIndex
			break
	else:
		GemRB.Log (LOG_WARNING, "IsDualClassed", "Invalid dualclass combination, treating as a single class!")
		print(DualedFrom, Multi, KitIndex, FirstClassIndex)
		return (0, -1, -1)

	if KittedClassIndex == FirstClassIndex and KitIndex:
		return (1, KitIndex, SecondClassIndex)
	elif KittedClassIndex == SecondClassIndex:
		return (3, FirstClassIndex, KitIndex)
	else:
		return (2, FirstClassIndex, SecondClassIndex)

def IsDualSwap (actor, override=None):
	"""Returns true if the dualed classes are reverse of expectation.

	This can happen, because the engine gives dualclass characters the same ID as
	their multiclass counterpart (eg. FIGHTER_MAGE = 3). Logic would dictate that
	the new and old class levels would be stored in IE_LEVEL and IE_LEVEL2,
	respectively; however, if one duals from a fighter to a mage in the above
	example, the levels would actually be in reverse of expectation."""

	Dual = IsDualClassed (actor, 1)
	if override:
		CI1 = CommonTables.Classes.FindValue ("ID", override["old"])
		CI2 = CommonTables.Classes.FindValue ("ID", override["new"])
		Dual = (2, CI1, CI2) # TODO: support IsDualClassed mode 3 once a gui for it is added

	# not dual classed
	if Dual[0] == 0:
		return 0

	# split the full class name into its individual parts
	# i.e FIGHTER_MAGE becomes [FIGHTER, MAGE]
	Class = GetClassRowName(actor).split("_")
	if override:
		Class = GetClassRowName(override["mc"], "class").split("_")

	# get our old class name
	if Dual[0] > 1:
		BaseClass = GetClassRowName(Dual[1], "index")
	else:
		BaseClass = CommonTables.KitList.GetValue (Dual[1], 7)
		if BaseClass == "*":
			# mod boilerplate
			return 0
		BaseClass = GetClassRowName(BaseClass, "class")

	# if our old class is the first class, we need to swap
	if Class[0] == BaseClass:
		return 1

	return 0

def IsMultiClassed (actor, verbose):
	"""Returns a tuple containing the multiclass information.

	Return[0] contains the total number of classes.
	Return[1-3] contain the ID of their respective classes.
	If verbose is false, only Return[0] has useable data."""

	# change this if it will ever be needed
	if GameCheck.IsIWD2():
		return (0,-1,-1,-1)

	# get our base class
	IsMulti = HasMultiClassBits (actor)
	IsDual = IsDualClassed (actor, 0)

	# dual-class char's look like multi-class chars
	if (IsMulti == 0) or (IsDual[0] > 0):
		return (0,-1,-1,-1)
	elif verbose == 0:
		return (IsMulti,-1,-1,-1)

	# get all our classes (leave space for our number of classes in the return array)
	Classes = [0]*3
	NumClasses = 0
	Mask = 1 # we're looking at multiples of 2
	ClassNames = GetClassRowName(actor).split("_")

	# loop through each class and test it as a mask
	ClassCount = CommonTables.Classes.GetRowCount()
	for i in range (1, ClassCount):
		if IsMulti & Mask == 0:
			Mask = 1 << i
			continue

		# we need to place the classes in the array based on their order in the name,
		# NOT the order they are detected in
		CurrentName = GetClassRowName (i, "class")
		if CurrentName == "*":
			# we read too far, as the upper range limit is greater than the number of "single" classes
			break
		for j in range(len(ClassNames)):
			if ClassNames[j] == CurrentName:
				Classes[j] = i # mask is (i-1)^2 where i is class id
		NumClasses = NumClasses + 1
		Mask = 1 << i

	# in case we couldn't figure out to which classes the multi belonged
	if NumClasses < 2:
		GemRB.Log (LOG_ERROR, "IsMultiClassed", "Couldn't figure out the individual classes of multiclass!")
		print(ClassNames)
		return (0,-1,-1,-1)

	# return the tuple
	return (NumClasses, Classes[0], Classes[1], Classes[2])

def IsNamelessOne (actor):
	# A very simple test to identify the actor as TNO
	if not GameCheck.IsPST():
		return False

	Specific = GemRB.GetPlayerStat (actor, IE_SPECIFIC)
	if Specific == 2:
		return True

	return False

def NamelessOneClass (actor):
	# A shortcut function to determine the identity of the nameless one 
	# and get his class if that is the case
	if IsNamelessOne(actor):
		return GetClassRowName(actor)

	return None

def CanDualClass(actor):
	# race restriction (human)
	RaceName = CommonTables.Races.FindValue ("ID", GemRB.GetPlayerStat (actor, IE_RACE, 1))
	if not RaceName:
		return 1
	RaceName = CommonTables.Races.GetRowName (RaceName)
	RaceDual = CommonTables.Races.GetValue (RaceName, "CANDUAL")
	if int(RaceDual) != 1:
		return 1

	# already dualclassed
	Dual = IsDualClassed (actor,0)
	if Dual[0] > 0:
		return 1

	DualClassTable = GemRB.LoadTable ("dualclas")
	ClassName = GetClassRowName(actor)
	KitIndex = GetKitIndex (actor)
	if KitIndex == 0:
		ClassTitle = ClassName
	else:
		ClassTitle = CommonTables.KitList.GetValue (KitIndex, 0)
	Row = DualClassTable.GetRowIndex (ClassTitle)
	if Row == None and KitIndex > 0:
		# retry with the baseclass in case the kit/school is missing (eg. wildmages)
		Row = DualClassTable.GetRowIndex (ClassName)

	if Row == None:
		print("CannotDualClass: inappropriate starting class")
		return 1

	# create a lookup table for the DualClassTable columns
	classes = []
	for col in range(DualClassTable.GetColumnCount()):
		classes.append(DualClassTable.GetColumnName(col))

	matches = []
	Sum = 0
	for col in range (0, DualClassTable.GetColumnCount ()):
		value = DualClassTable.GetValue (Row, col)
		Sum += value
		if value == 1:
			matches.append (classes[col])

	# cannot dc if all the columns of the DualClassTable are 0
	if Sum == 0:
		print("CannotDualClass: all the columns of the DualClassTable are 0")
		return 1

	# if the only choice for dc is already the same as the actors base class
	if Sum == 1 and ClassName in matches and KitIndex == 0:
		print("CannotDualClass: the only choice for dc is already the same as the actors base class")
		return 1

	AlignmentTable = GemRB.LoadTable ("alignmnt")
	Alignment = GemRB.GetPlayerStat (actor, IE_ALIGNMENT)
	AlignmentColName = CommonTables.Aligns.FindValue (3, Alignment)
	AlignmentColName = CommonTables.Aligns.GetValue (AlignmentColName, 4)
	if AlignmentColName == -1:
		print("CannotDualClass: extraordinary character alignment")
		return 1
	Sum = 0
	for classy in matches:
		Sum += AlignmentTable.GetValue (classy, AlignmentColName)

	# cannot dc if all the available classes forbid the chars alignment
	if Sum == 0:
		print("CannotDualClass: all the available classes forbid the chars alignment")
		return 1

	# check current class' stat limitations
	CurrentStatTable = GemRB.LoadTable ("abdcscrq")
	ClassStatIndex = CurrentStatTable.GetRowIndex (ClassTitle)
	if ClassStatIndex == None and KitIndex > 0:
		# retry with the baseclass in case the kit/school is missing (eg. wildmages)
		ClassStatIndex = CurrentStatTable.GetRowIndex (ClassName)
	for stat in range (6):
		minimum = CurrentStatTable.GetValue (ClassStatIndex, stat)
		name = CurrentStatTable.GetColumnName (stat)
		if GemRB.GetPlayerStat (actor, SafeStatEval ("IE_" + name[4:]), 1) < minimum:
			print("CannotDualClass: current class' stat limitations are too big")
			return 1

	# check new class' stat limitations - make sure there are any good class choices
	TargetStatTable = GemRB.LoadTable ("abdcdsrq")
	bad = 0
	for match in matches:
		ClassStatIndex = TargetStatTable.GetRowIndex (match)
		for stat in range (6):
			minimum = TargetStatTable.GetValue (ClassStatIndex, stat)
			name = TargetStatTable.GetColumnName (stat)
			if GemRB.GetPlayerStat (actor, SafeStatEval ("IE_" + name[4:]), 1) < minimum:
				bad += 1
				break
	if len(matches) == bad:
		print("CannotDualClass: no good new class choices")
		return 1

	# must be at least level 2
	if GemRB.GetPlayerStat (actor, IE_LEVEL) == 1:
		print("CannotDualClass: level 1")
		return 1
	return 0

def IsWarrior (actor):
	IsWarrior = CommonTables.ClassSkills.GetValue (GetClassRowName(actor), "NO_PROF")

	# warriors get only a -2 penalty for wielding weapons they are not proficient with
	# FIXME: make the check more robust, someone may change the value!
	IsWarrior = (IsWarrior == -2)

	Dual = IsDualClassed (actor, 0)
	if Dual[0] > 0:
		DualedFrom = GemRB.GetPlayerStat (actor, IE_MC_FLAGS) & MC_WAS_ANY_CLASS
		FirstClassIndex = CommonTables.Classes.FindValue ("MC_WAS_ID", DualedFrom)
		FirstClassName = CommonTables.Classes.GetRowName (FirstClassIndex)
		OldIsWarrior = CommonTables.ClassSkills.GetValue (FirstClassName, "NO_PROF")
		# there are no warrior to warrior dualclasses, so if the previous class was one, the current one certainly isn't
		if OldIsWarrior == -2:
			return 0
		# but there are also non-warrior to non-warrior dualclasses, so just use the new class check

	return IsWarrior

def SetupDamageInfo (pc, Button, Window):
	hp = GemRB.GetPlayerStat (pc, IE_HITPOINTS)
	hp_max = GemRB.GetPlayerStat (pc, IE_MAXHITPOINTS)
	state = GemRB.GetPlayerStat (pc, IE_STATE_ID)

	if hp_max < 1 or hp == "?":
		ratio = 0.0
	else:
		ratio = hp / float(hp_max)

	if hp < 1 or (state & STATE_DEAD):
		c = {'r' : 64, 'g' : 64, 'b' : 64, 'a' : 255}
		Button.SetOverlay (0, c, c)

	if ratio == 1.0:
		band = 0
		color = {'r' : 255, 'g' : 255, 'b' : 255}  # white
	elif ratio >= 0.75:
		band = 1
		color = {'r' : 0, 'g' : 255, 'b' : 0}  # green
	elif ratio >= 0.50:
		band = 2
		color = {'r' : 255, 'g' : 255, 'b' : 0}  # yellow
	elif ratio >= 0.25:
		band = 3
		color = {'r' : 255, 'g' : 128, 'b' : 0}  # orange
	else:
		band = 4
		color = {'r' : 255, 'g' : 0, 'b' : 0}  # red

	if GemRB.GetVar("Old Portrait Health") or not GameCheck.IsIWD2():
		# draw the blood overlay
		if hp >= 1 and not (state & STATE_DEAD):
			c1 = {'r' : 0x70, 'g' : 0, 'b' : 0, 'a' : 0xff}
			c2 = {'r' : 0xf7, 'g' : 0, 'b' : 0, 'a' : 0xff}
			Button.SetOverlay (ratio, c1, c2)
	else:
		# scale the hp bar under the portraits and recolor it
		# GUIHITPT has 5 frames with different severity colors
		# luckily their ids follow a nice pattern
		hpBar = Window.GetControl (pc-1 + 50)
		hpBar.SetBAM ("GUIHITPT", band, 0)
		hpBar.SetPictureClipping (ratio)
		hpBar.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	ratio_str = ""
	if hp != "?":
		ratio_str = "\n%d/%d" %(hp, hp_max)
	Button.SetTooltip (GemRB.GetPlayerName (pc, 1) + ratio_str)

	return ratio_str, color

# set MAGESCHOOL to mage school (kit) index
def UpdateMageSchool(pc):
	GemRB.SetVar ("MAGESCHOOL", 0)
	Kit = GetKitIndex (pc)
	if Kit and CommonTables.KitList.GetValue (Kit, 7) == 1:
		MageTable = GemRB.LoadTable ("magesch")
		GemRB.SetVar ("MAGESCHOOL", MageTable.GetRowIndex (CommonTables.KitList.GetValue (Kit, 0, GTV_STR)))

def SetCurrentDateTokens (stat, plural=False):
	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime ()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime // 15)
	days = party_time // 7200
	hours = (party_time % 7200) // 300

	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))
	if plural:
		return

	# construct <GAMEDAYS> days ~and~ ~<HOUR> hours~
	if days == 1:
		time = GemRB.GetString (10698)
	else:
		time = GemRB.GetString (10697)
	time += " " + GemRB.GetString (10699) + " "
	if days == 0:
		# only display hours
		time = ""

	if hours == 1:
		time += GemRB.GetString (10701)
	else:
		time += GemRB.GetString (10700)

	return time

def SetSaveDir():
	if GameCheck.IsIWD1() or GameCheck.IsIWD2():
		GemRB.SetToken ("SaveDir", "mpsave")
	elif GameCheck.IsBG1() and GemRB.GetVar ("PlayMode") == 1:
		GemRB.SetToken ("SaveDir", "mpsave")
	else:
		GemRB.SetToken ("SaveDir", "save")

# gray out window or mark it as visible depending on the actor's state
# Always greys it out for actors that are: dead, berserking
# The third parameter is another check which must be 0 to maintain window visibility
def AdjustWindowVisibility (Window, pc, additionalCheck):
	if not additionalCheck and GemRB.ValidTarget (pc, GA_SELECT|GA_NO_DEAD):
		Window.SetDisabled (False)
	else:
		Window.SetDisabled (True)
	return

def UsingTouchInput ():
	return GemRB.GetSystemVariable (SV_TOUCH)

# return ceil(n/d)
#
def ceildiv (n, d):
	if d == 0:
		raise ZeroDivisionError("ceildiv by zero")
	elif d < 0:
		return (n + d + 1) // d
	else:
		return (n + d - 1) // d

# a placeholder for unimplemented and hardcoded key actions
def ResolveKey():
	return

# eval that only accepts alphanumerics and "_"
# used for converting constructed stat names to their values, eg. IE_STR to 36
def SafeStatEval (expression):
	# if we ever import string: string.ascii_letters + "_" + string.digits
	alnum = 'abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
	for chr in expression:
		if chr not in alnum:
			raise ValueError("Invalid input! Bad data encountered, check the GemRB install's integrity!")

	return eval(expression)

GameWindow = GUIClasses.GWindow(ID=0, SCRIPT_GROUP="GAMEWIN")
GameControl = GUIClasses.GView(ID=0, SCRIPT_GROUP="GC")

def DisplayAC (pc, window, labelID):
	AC = GemRB.GetPlayerStat (pc, IE_ARMORCLASS) + GetACStyleBonus (pc)
	Label = window.GetControl (labelID)
	Label.SetText (str (AC))
	Label.SetTooltip (17183)

def GetACStyleBonus (pc):
	stars = GemRB.GetPlayerStat(pc, IE_PROFICIENCYSINGLEWEAPON) & 0x7
	if not stars:
		return 0

	WStyleTable = GemRB.LoadTable ("wssingle", 1)
	if not WStyleTable:
		return 0
	# are we actually single-wielding?
	cdet = GemRB.GetCombatDetails (pc, 0)
	if cdet["Style"] % 1000 != IE_PROFICIENCYSINGLEWEAPON:
		return 0
	return WStyleTable.GetValue (str(stars), "AC")

def AddDefaultVoiceSet (VoiceList, Voices):
	if GameCheck.IsBG1 () or GameCheck.IsBG2 ():
		Options = collections.OrderedDict(enumerate(Voices))
		Options[-1] = "default"
		Options = collections.OrderedDict(sorted(Options.items()))
		VoiceList.SetOptions (list(Options.values()))
		return True
	return False

def OverrideDefaultVoiceSet (Gender, CharSound):
	# handle "default" gendered voice
	if CharSound == "default" and not GemRB.HasResource ("defaulta", RES_WAV):
		if GameCheck.IsBG1 ():
			Gender2Sound = [ "", "mainm", "mainf" ]
			CharSound = Gender2Sound[Gender]
		elif GameCheck.IsBG2 ():
			Gender2Sound = [ "", "male005", "female4" ]
			CharSound = Gender2Sound[Gender]
	return CharSound

# -*-python-*-
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
import CommonTables
from ie_restype import RES_CHU, RES_2DA
from ie_spells import LS_MEMO
from GUIDefines import *
from ie_stats import *
from ie_slots import SLOT_ALL
from ie_feats import FEAT_STRONG_BACK

OtherWindowFn = None
NextWindowFn = None

CommonTables.Load ()

def CloseOtherWindow (NewWindowFn):
	global OtherWindowFn,NextWindowFn

	GemRB.LeaveContainer()
	if OtherWindowFn and OtherWindowFn != NewWindowFn:
		# allow detection of 'next window'
		NextWindowFn = NewWindowFn
		# switching from a window to something else, call old function
		OtherWindowFn ()
		OtherWindowFn = NewWindowFn
		return 0
	elif OtherWindowFn:
		# something is calling us with its own function, so
		# it is closing down, return true
		OtherWindowFn = None
		return 1
	else:
		# new window, no need to do setup
		OtherWindowFn = NewWindowFn
		NextWindowFn = None
		return 0

def GetWindowPack():
	width = GemRB.GetSystemVariable (SV_WIDTH)
	height = GemRB.GetSystemVariable (SV_HEIGHT)

	if GemRB.GameType == "pst":
		default = "GUIWORLD"
	else:
		default = "GUIW"

	# use a custom gui if there is one
	gui = "CGUI" + str(width)[:2] + str(height)[:2]
	if GemRB.HasResource (gui, RES_CHU, 1):
		return gui

	gui = None
	if width == 640:
		gui = default
	elif width == 800:
		gui = "GUIW08"
	elif width == 1024:
		gui = "GUIW10"
	elif width == 1280:
		gui = "GUIW12"
	if gui:
		if GemRB.HasResource (gui, RES_CHU, 1):
			return gui

	# fallback to the smallest resolution
	return default

def LocationPressed ():
	AreaInfo = GemRB.GetAreaInfo()
	print( "%s [%d.%d]\n"%(AreaInfo["CurrentArea"], AreaInfo["PositionX"], AreaInfo["PositionY"]) )
	return

def SelectFormation ():
	GemRB.GameSetFormation (GemRB.GetVar ("Formation"))
	return

def OpenFloatMenuWindow (x, y):
	if GameCheck.IsPST():
		import FloatMenuWindow
		FloatMenuWindow.OpenFloatMenuWindow(x, y)
	else:
		GemRB.GameControlSetTargetMode (TARGET_MODE_NONE)

def GetActorPaperDoll (actor):
	anim_id = GemRB.GetPlayerStat (actor, IE_ANIMATION_ID)
	level = GemRB.GetPlayerStat (actor, IE_ARMOR_TYPE)
	row = "0x%04X" %anim_id
	which = "LEVEL%d" %(level+1)
	doll = CommonTables.Pdolls.GetValue (row, which)
	if doll == "*":
		print "GetActorPaperDoll: Missing paper doll for animation", row, which
	return doll

def SelectAllOnPress ():
	GemRB.GameSelectPC (0, 1)

def GearsClicked ():
	#GemRB.SetPlayerStat(GemRB.GameGetFirstSelectedPC (),44,249990)
	GemRB.GamePause (2, 0)

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
	days = currentTime / 7200
	hours = (currentTime % 7200) / 300
	GemRB.SetToken ('GAMEDAY', str (days))
	GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

def Gain(infostr, ability):
	GemRB.SetToken ('SPECIALABILITYNAME', GemRB.GetString(int(ability) ) )
	GemRB.DisplayString (infostr)

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
	import Spellbook

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
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
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
					print "ERROR, unknown class ability (type): ", ab

def MakeSpellCount (pc, spell, count):
	have = GemRB.CountSpells (pc, spell, 1)
	if count<=have:
		return
	for i in range (count-have):
		GemRB.LearnSpell (pc, spell, LS_MEMO)
	return
	
# remove all class abilities up to the given level
# for dual-classing mainly
def RemoveClassAbilities (pc, table, Level):
	TmpTable = GemRB.LoadTable (table)
	import Spellbook

	# gotta stay positive
	if Level < 0:
		return

	# make sure we don't go out too far
	jMax = Level
	if jMax > TmpTable.GetColumnCount ():
		jMax = TmpTable.GetColumnCount ()

	for i in range(TmpTable.GetRowCount ()):
		for j in range (jMax):
			ab = TmpTable.GetValue (i, j, 0)
			if ab and ab != "****":
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
					print "ERROR, unknown class ability (type): ", ab

def UpdateInventorySlot (pc, Button, Slot, Type, Equipped=False):
	Button.SetFont ("NUMBER")
	Button.SetBorder (0, 0,0,0,0, 128,128,255,64, 0,1)
	Button.SetBorder (1, 2,2,2,2, 32,32,255,0, 0,0)
	Button.SetBorder (2, 0,0,0,0, 255,128,128,64, 0,1)
	Button.SetText ("")
	Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_PICTURE, OP_OR)

	if Slot == None:
		Button.SetFlags (IE_GUI_BUTTON_PICTURE, OP_NAND)
		if Type == "inventory":
			Button.SetTooltip (12013) # Personal Item
		elif Type == "ground":
			Button.SetTooltip (12011) # Ground Item
		else:
			Button.SetTooltip ("")
		Button.EnableBorder (0, 0)
		Button.EnableBorder (1, 0)
		Button.EnableBorder (2, 0)
	else:
		item = GemRB.GetItem (Slot['ItemResRef'])
		identified = Slot["Flags"] & IE_INV_ITEM_IDENTIFIED
		magical = Slot["Flags"] & IE_INV_ITEM_MAGICAL

		# MaxStackAmount holds the *maximum* item count in the stack while Usages0 holds the actual
		if item["MaxStackAmount"] > 1:
			Button.SetText (str (Slot["Usages0"]))
		else:
			Button.SetText ("")

		# auto-identify mundane items; the actual indentification will happen on transfer
		if not identified and item["LoreToID"] == 0:
			identified = True

		if not identified or item["ItemNameIdentified"] == -1:
			Button.SetTooltip (item["ItemName"])
			Button.EnableBorder (0, 1)
			Button.EnableBorder (1, 0)
		else:
			Button.SetTooltip (item["ItemNameIdentified"])
			Button.EnableBorder (0, 0)
			if magical:
				Button.EnableBorder (1, 1)
			else:
				Button.EnableBorder (1, 0)

		if GemRB.CanUseItemType (SLOT_ALL, Slot['ItemResRef'], pc, Equipped):
			Button.EnableBorder (2, 0)
		else:
			Button.EnableBorder (2, 1)

		Button.SetItemIcon (Slot['ItemResRef'], 0)

	return

# PST uses a button, IWD2 two types, the rest are the same with two labels
def SetEncumbranceLabels (Window, ControlID, Control2ID, pc, invert_colors = False):
	"""Displays the encumbrance as a ratio of current to maximum."""

	# encumbrance
	encumbrance = GemRB.GetPlayerStat (pc, IE_ENCUMBRANCE)
	max_encumb = GemRB.GetMaxEncumbrance (pc)

	Control = Window.GetControl (ControlID)
	if GameCheck.IsPST():
		# FIXME: there should be a space before LB symbol (':')
		Control.SetText (str (encumbrance) + ":\n\n" + str (max_encumb) + ":")
	elif GameCheck.IsIWD2() and not Control2ID:
		Control.SetText (str (encumbrance) + "/" + str(max_encumb) + GemRB.GetString(39537))
	else:
		Control.SetText (str (encumbrance) + ":")
		if not Control2ID: # shouldn't happen
			print "Missing second control parameter to SetEncumbranceLabels!"
			return
		Control2 = Window.GetControl (Control2ID)
		Control2.SetText (str (max_encumb) + ":")

	ratio = (0.0 + encumbrance) / max_encumb
	if GameCheck.IsIWD2 () or GameCheck.IsPST ():
		if ratio > 1.0:
			if invert_colors:
				Control.SetTextColor (255, 0, 0, True)
			else:
				Control.SetTextColor (255, 0, 0)
		elif ratio > 0.8:
			if invert_colors:
				Control.SetTextColor (255, 255, 0, True)
			else:
				Control.SetTextColor (255, 255, 0)
		else:
			if invert_colors:
				Control.SetTextColor (255, 255, 255, True)
			else:
				Control.SetTextColor (255, 255, 255)

		if Control2ID:
			Control2.SetTextColor (255, 0, 0)

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

	if ClassTitle == 0:
		ClassName = GetClassRowName (actor)
		KitIndex = GetKitIndex (actor)
		Multi = HasMultiClassBits (actor)
		Dual = IsDualClassed (actor, 1)

		if Multi and Dual[0] == 0: # true multi class
			ClassTitle = CommonTables.Classes.GetValue (ClassName, "CAP_REF")
			ClassTitle = GemRB.GetString (ClassTitle)
		else:
			if Dual[0]: # dual class
				# first (previous) kit or class of the dual class
				if Dual[0] == 1:
					ClassTitle = CommonTables.KitList.GetValue (Dual[1], 2)
				elif Dual[0] == 2:
					ClassTitle = CommonTables.Classes.GetValue (GetClassRowName(Dual[1], "index"), "CAP_REF")
				ClassTitle = GemRB.GetString (ClassTitle) + " / "
				ClassTitle += GemRB.GetString (CommonTables.Classes.GetValue (GetClassRowName(Dual[2], "index"), "CAP_REF"))
			else: # ordinary class or kit
				if KitIndex:
					ClassTitle = CommonTables.KitList.GetValue (KitIndex, 2)
				else:
					ClassTitle = CommonTables.Classes.GetValue (ClassName, "CAP_REF")
				if ClassTitle != "*":
					ClassTitle = GemRB.GetString (ClassTitle)
	else:
		ClassTitle = GemRB.GetString (ClassTitle)

	#GetActorClassTitle returns string now...
	#if ClassTitle == "*":
	#	return 0

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
		if KitIndex == -1:
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
			raise RuntimeError, "Bad type parameter for GetClassRowName: ", which
		ClassIndex = CommonTables.Classes.FindValue ("ID", Class)
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

	Return[0] is 0 if not dualclassed, 1 if the old class is a kit, 2 otherwise.
	Return[1] contains either the kit or class index of the old class.
	Return[2] contains the class index of the new class.
	If verbose is false, only Return[0] contains useable data."""

	if GameCheck.IsIWD2():
		return (0,-1,-1)

	Multi = HasMultiClassBits (actor)

	if Multi == 0:
		return (0, -1, -1)

	DualedFrom = GemRB.GetPlayerStat (actor, IE_MC_FLAGS) & MC_WAS_ANY_CLASS

	if verbose:
		DualInfo = []
		KitIndex = GetKitIndex (actor)

		if DualedFrom > 0: # first (previous) class of the dual class
			FirstClassIndex = CommonTables.Classes.FindValue ("MC_WAS_ID", DualedFrom)
			if KitIndex:
				DualInfo.append (1)
				DualInfo.append (KitIndex)
			else:
				DualInfo.append (2)
				DualInfo.append (FirstClassIndex)

			# use the first class of the multiclass bunch that isn't the same as the first class
			Mask = 1
			for i in range (1,16):
				if Multi & Mask:
					ClassIndex = CommonTables.Classes.FindValue ("ID", i)
					if ClassIndex == FirstClassIndex:
						Mask = 1 << i
						continue
					DualInfo.append (ClassIndex)
					break
				Mask = 1 << i
			if len(DualInfo) != 3:
				print "WARNING: Invalid dualclass combination, treating as a single class!"
				print DualedFrom, Multi, KitIndex, DualInfo
				return (0,-1,-1)

			return DualInfo
		else:
			return (0,-1,-1)
	else:
		if DualedFrom > 0:
			return (1,-1,-1)
		else:
			return (0,-1,-1)

def IsDualSwap (actor):
	"""Returns true if the dualed classes are reverse of expection.

	This can happen, because the engine gives dualclass characters the same ID as
	their multiclass counterpart (eg. FIGHTER_MAGE = 3). Logic would dictate that
	the new and old class levels would be stored in IE_LEVEL and IE_LEVEL2,
	respectively; however, if one duals from a fighter to a mage in the above
	example, the levels would actually be in reverse of expectation."""

	Dual = IsDualClassed (actor, 1)

	# not dual classed
	if Dual[0] == 0:
		return 0

	# split the full class name into its individual parts
	# i.e FIGHTER_MAGE becomes [FIGHTER, MAGE]
	Class = GetClassRowName(actor).split("_")

	# get our old class name
	if Dual[0] == 2:
		BaseClass = GetClassRowName(Dual[1], "index")
	else:
		BaseClass = GetKitIndex (actor)
		BaseClass = CommonTables.KitList.GetValue (BaseClass, 7)
		if BaseClass == "*":
			# mod boilerplate
			return 0
		BaseClass = GetClassRowName(BaseClass, "index")

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
		if IsMulti&Mask: # it's part of this class
			#we need to place the classes in the array based on their order in the name,
			#NOT the order they are detected in
			CurrentName = GetClassRowName (i, "class")
			if CurrentName == "*":
				# we read too far, as the upper range limit is greater than the number of "single" classes
				break
			for j in range(len(ClassNames)):
				if ClassNames[j] == CurrentName:
					Classes[j] = i # mask is (i-1)^2 where i is class id
			NumClasses = NumClasses+1
		Mask = 1 << i # shift to the next multiple of 2 for testing

	# in case we couldn't figure out to which classes the multi belonged
	if NumClasses < 2:
		print "ERROR: couldn't figure out the individual classes of multiclass", ClassNames
		return (0,-1,-1,-1)

	# return the tuple
	return (NumClasses, Classes[0], Classes[1], Classes[2])

def CanDualClass(actor):
	# race restriction (human)
	RaceName = CommonTables.Races.FindValue ("ID", GemRB.GetPlayerStat (actor, IE_RACE, 1))
	RaceName = CommonTables.Races.GetRowName (RaceName)
	RaceDual = CommonTables.Races.GetValue (RaceName, "CANDUAL")
	if int(RaceDual) != 1:
		return 1

	# already dualclassed
	Dual = IsDualClassed (actor,0)
	if Dual[0] > 0:
		return 1

	DualClassTable = GemRB.LoadTable ("dualclas")
	CurrentStatTable = GemRB.LoadTable ("abdcscrq")
	ClassName = GetClassRowName(actor)
	KitIndex = GetKitIndex (actor)
	if KitIndex == 0:
		ClassTitle = ClassName
	else:
		ClassTitle = CommonTables.KitList.GetValue (KitIndex, 0)
	Row = DualClassTable.GetRowIndex (ClassTitle)

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
		print "CannotDualClass: all the columns of the DualClassTable are 0"
		return 1

	# if the only choice for dc is already the same as the actors base class
	if Sum == 1 and ClassName in matches and KitIndex == 0:
		print "CannotDualClass: the only choice for dc is already the same as the actors base class"
		return 1

	AlignmentTable = GemRB.LoadTable ("alignmnt")
	Alignment = GemRB.GetPlayerStat (actor, IE_ALIGNMENT)
	AlignmentColName = CommonTables.Aligns.FindValue (3, Alignment)
	AlignmentColName = CommonTables.Aligns.GetValue (AlignmentColName, 4)
	if AlignmentColName == -1:
		print "CannotDualClass: extraordinary character alignment"
		return 1
	Sum = 0
	for classy in matches:
		Sum += AlignmentTable.GetValue (classy, AlignmentColName)

	# cannot dc if all the available classes forbid the chars alignment
	if Sum == 0:
		print "CannotDualClass: all the available classes forbid the chars alignment"
		return 1

	# check current class' stat limitations
	ClassStatIndex = CurrentStatTable.GetRowIndex (ClassTitle)
	for stat in range (6):
		minimum = CurrentStatTable.GetValue (ClassStatIndex, stat)
		name = CurrentStatTable.GetColumnName (stat)
		if GemRB.GetPlayerStat (actor, eval ("IE_" + name[4:])) < minimum:
			print "CannotDualClass: current class' stat limitations are too big"
			return 1

	# check new class' stat limitations - make sure there are any good class choices
	TargetStatTable = GemRB.LoadTable ("abdcdsrq")
	bad = 0
	for match in matches:
		ClassStatIndex = TargetStatTable.GetRowIndex (match)
		for stat in range (6):
			minimum = TargetStatTable.GetValue (ClassStatIndex, stat)
			name = TargetStatTable.GetColumnName (stat)
			if GemRB.GetPlayerStat (actor, eval ("IE_" + name[4:])) < minimum:
				bad += 1
				break
	if len(matches) == bad:
		print "CannotDualClass: no good new class choices"
		return 1

	# must be at least level 2
	if GemRB.GetPlayerStat (actor, IE_LEVEL) == 1:
		print "CannotDualClass: level 1"
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

	if hp_max < 1:
		ratio = 0.0
	else:
		ratio = (hp+0.0) / hp_max

	if hp < 1 or (state & STATE_DEAD):
		Button.SetOverlay (0, 64,64,64,200, 64,64,64,200)

	if ratio == 1:
		band = 0
		color = (255, 255, 255)  # white
	elif ratio >= 0.75:
		band = 1
		color = (0, 255, 0)  # green
	elif ratio >= 0.50:
		band = 2
		color = (255, 255, 0)  # yellow
	elif ratio >= 0.25:
		band = 3
		color = (255, 128, 0)  # orange
	else:
		band = 4
		color = (255, 0, 0)  # red

	if GemRB.GetVar("Old Portrait Health") or not GameCheck.IsIWD2():
		# draw the blood overlay
		if hp >= 1 and not (state & STATE_DEAD):
			Button.SetOverlay (ratio, 160,0,0,200, 60,0,0,190)
	else:
		# scale the hp bar under the portraits and recolor it
		# GUIHITPT has 5 frames with different severity colors
		# luckily their ids follow a nice pattern
		hpBar = Window.GetControl (pc-1 + 50)
		hpBar.SetBAM ("GUIHITPT", band, 0)
		hpBar.SetPictureClipping (ratio)
		hpBar.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_OR)

	ratio_str = "\n%d/%d" %(hp, hp_max)
	Button.SetTooltip (GemRB.GetPlayerName (pc, 1) + ratio_str)

	return ratio_str, color

def SetCurrentDateTokens (stat):
	# NOTE: currentTime is in seconds, joinTime is in seconds * 15
	#   (script updates). In each case, there are 60 seconds
	#   in a minute, 24 hours in a day, but ONLY 5 minutes in an hour!!
	# Hence currentTime (and joinTime after div by 15) has
	#   7200 secs a day (60 * 5 * 24)
	currentTime = GemRB.GetGameTime ()
	joinTime = stat['JoinDate'] - stat['AwayTime']

	party_time = currentTime - (joinTime / 15)
	days = party_time / 7200
	hours = (party_time % 7200) / 300

	# it is true, they changed the token
	if GameCheck.IsBG2():
		GemRB.SetToken ('GAMEDAY', str (days))
	else:
		GemRB.SetToken ('GAMEDAYS', str (days))
	GemRB.SetToken ('HOUR', str (hours))

	return (days, hours)

# gray out window or mark it as visible depending on the actor's state
# Always greys it out for actors that are: dead, berserking
# The third parameter is another check which must be 0 to maintain window visibility
def AdjustWindowVisibility (Window, pc, additionalCheck):
	if not additionalCheck and GemRB.ValidTarget (pc, GA_SELECT|GA_NO_DEAD):
		Window.SetVisible (WINDOW_VISIBLE)
	else:
		Window.SetVisible (WINDOW_GRAYED)
	return

# return ceil(n/d)
# 
def ceildiv (n, d):
	if d == 0:
		raise ZeroDivisionError("ceildiv by zero")
	elif d < 0:
		return (n+d+1)/d
	else:
		return (n+d-1)/d

# a placeholder for unimplemented and hardcoded key actions
def ResolveKey():
	return

GameWindow = GUIClasses.GWindow(0)
GameControl = GUIClasses.GControl(0,0)

class _stdioWrapper(object):
	def __init__(self, log_level):
		self.log_level = log_level
		self.buffer = ""
	def write(self, message):
		self.buffer += str(message)
		if self.buffer.endswith("\n"):
			out = self.buffer.rstrip("\n")
			if out:
				GemRB.Log(self.log_level, "Python", out)
			self.buffer = ""

def _wrapStdio():
	import sys
	sys.stdout = _stdioWrapper(LOG_MESSAGE)
	sys.stderr = _stdioWrapper(LOG_ERROR)

_wrapStdio()

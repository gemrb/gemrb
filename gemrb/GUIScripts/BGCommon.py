import GemRB

from GUICommon import *
from ie_stats import *
from ie_modal import *
from ie_action import *
from ie_slots import *

print
ClassTable = GemRB.LoadTableObject ("classes")
KitListTable = GemRB.LoadTableObject ("kitlist")
ClassSkillsTable = GemRB.LoadTableObject ("clskills")
RaceTable = GemRB.LoadTableObject ("races")
NextLevelTable = GemRB.LoadTableObject ("xplevel")

def GetKitIndex (actor):
	"""Return the index of the actors kit from KITLIST.2da.

	Returns 0 if the class is not kitted."""

	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Kit = GemRB.GetPlayerStat (actor, IE_KIT)
	KitIndex = 0

	if Kit & 0xc000 == 0x4000:
		KitIndex = Kit & 0xfff

	# carefully looking for kit by the usability flag
	# since the barbarian kit id clashes with the no-kit value
	if KitIndex == 0 and Kit != 0x4000:
		KitIndex = KitListTable.FindValue (6, Kit)
		if KitIndex == -1:
			KitIndex = 0

	return KitIndex

def IsDualClassed(actor, verbose):
	"""Returns an array containing the dual class information.

	Return[0] is 0 if not dualclassed, 1 if the old class is a kit, 2 otherwise.
	Return[1] contains either the kit or class index of the old class.
	Return[2] contains the class index of the new class.
	If verbose is false, only Return[0] contains useable data."""

	Dual = GemRB.GetPlayerStat (actor, IE_MC_FLAGS)
	Dual = Dual & ~(MC_EXPORTABLE|MC_PLOT_CRITICAL|MC_BEENINPARTY|MC_HIDDEN)

	if verbose:
		Class = GemRB.GetPlayerStat (actor, IE_CLASS)
		ClassIndex = ClassTable.FindValue (5, Class)
		Multi = ClassTable.GetValue (ClassIndex, 4)
		DualInfo = []
		KitIndex = GetKitIndex (actor)

		if (Dual & MC_WAS_ANY_CLASS) > 0: # first (previous) class of the dual class
			MCColumn = ClassTable.GetColumnIndex ("MC_WAS_ID")
			FirstClassIndex = ClassTable.FindValue (MCColumn, Dual & MC_WAS_ANY_CLASS)
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
					ClassIndex = ClassTable.FindValue (5, i)
					if ClassIndex == FirstClassIndex:
						Mask = 1 << i
						continue
					DualInfo.append (ClassIndex)
					break
				Mask = 1 << i
			return DualInfo
		else:
			return (0,-1,-1)
	else:
		if (Dual & MC_WAS_ANY_CLASS) > 0:
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
	Class = GemRB.GetPlayerStat (actor, IE_CLASS)
	Class = ClassTable.FindValue (5, Class)
	Class = ClassTable.GetRowName (Class)
	Class = Class.split("_")

	# get our old class name
	if Dual[0] == 2:
		BaseClass = ClassTable.GetRowName (Dual[1])
	else:
		BaseClass = GetKitIndex (actor)
		BaseClass = KitListTable.GetValue (BaseClass, 7)
		BaseClass = ClassTable.FindValue (5, BaseClass)
		BaseClass = ClassTable.GetRowName (BaseClass)

	# if our old class is the first class, we need to swap
	if Class[0] == BaseClass:
		return 1

	return 0

def IsMultiClassed (actor, verbose):
	"""Returns a tuple containing the multiclass information.

	Return[0] contains the total number of classes.
	Return[1-3] contain the ID of their respective classes.
	If verbose is false, only Return[0] has useable data."""

	# get our base class
	ClassIndex = ClassTable.FindValue (5, GemRB.GetPlayerStat (actor, IE_CLASS))
	IsMulti = ClassTable.GetValue (ClassIndex, 4) # 0 if not multi'd
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
	ClassNames = ClassTable.GetRowName(ClassIndex).split("_")

	# loop through each class and test it as a mask
	# TODO: make 16 dynamic? -- allows for custom classes (not just kits)
	for i in range (1, 16):
		if IsMulti&Mask: # it's part of this class
			#we need to place the classes in the array based on their order in the name,
			#NOT the order they are detected in
			CurrentName = ClassTable.GetRowName (ClassTable.FindValue (5, i));
			for j in range(len(ClassNames)):
				if ClassNames[j] == CurrentName:
					Classes[j] = i # mask is (i-1)^2 where i is class id
			NumClasses = NumClasses+1
		Mask = 1 << i # shift to the next multiple of 2 for testing

	# in case we couldn't figure out to which classes the multi belonged
	if NumClasses < 2:
		return (0,-1,-1,-1)

	# return the tuple
	return (NumClasses, Classes[0], Classes[1], Classes[2])

def RemoveKnownSpells (pc, type, level1=1, level2=1, noslots=0, kit=0):
	"""Removes all known spells of a given type between two spell levels.

	If noslots is true, all memorization counts are set to 0.
	Kit is used to identify the priest spell mask of the spells to be removed;
	this is only used when removing spells in a dualclass."""

	# choose the correct limit based upon class type
	if type == IE_SPELL_TYPE_WIZARD:
		limit = 9
	elif type == IE_SPELL_TYPE_PRIEST:
		limit = 7

		# make sure that we get the original kit, if we have one
		if kit:
			originalkit = GetKitIndex (pc)

			if originalkit: # kitted; find the class value
				originalkit = KitListTable.GetValue (originalkit, 7)
			else: # just get the class value
				originalkit = GemRB.GetPlayerStat (pc, IE_CLASS)

			# this is is specifically for dual-classes and will not work to remove only one
			# spell type from a ranger/cleric multi-class
			if ClassSkillsTable.GetValue (originalkit, 0, 0) != "*": # knows druid spells
				originalkit = 0x8000
			elif ClassSkillsTable.GetValue (originalkit, 1, 0) != "*": # knows cleric spells
				originalkit = 0x4000
			else: # don't know any other spells
				originalkit = 0

			# don't know how this would happen, but better to be safe
			if originalkit == kit:
				originalkit = 0
	elif type == IE_SPELL_TYPE_INNATE:
		limit = 1
	else: # can't do anything if an improper spell type is sent
		return 0

	# make sure we're within parameters
	if level1 < 1 or level2 > limit or level1 > level2:
		return 0

	# remove all spells for each level
	for level in range (level1-1, level2):
		# we need the count because we remove each spell in reverse order
		count = GemRB.GetKnownSpellsCount (pc, type, level)
		mod = count-1

		for spell in range (count):
			# see if we need to check for kit
			if type == IE_SPELL_TYPE_PRIEST and kit:
				# get the spells ref data
				ref = GemRB.GetKnownSpell (pc, type, level, mod-spell)
				ref = GemRB.GetSpell (ref['SpellResRef'], 1)

				# we have to look at the originalkit as well specifically for ranger/cleric dual-classes
				# we wouldn't want to remove all cleric spells and druid spells if we lost our cleric class
				# only the cleric ones
				if kit&ref['SpellDivine'] or (originalkit and not originalkit&ref['SpellDivine']):
					continue

			# remove the spell
			GemRB.RemoveSpell (pc, type, level, mod-spell)

		# remove memorization counts if disired
		if noslots:
			GemRB.SetMemorizableSpellsCount (pc, 0, type, level)

	# return success
	return 1

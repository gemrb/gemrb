# SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later
import GemRB
import GameCheck
from ie_restype import RES_2DA

# these two are only used in SetEncumbranceLabels, but that is called very often
StrMod = StrModEx = None
Classes = KitList = ClassSkills = Races = NextLevel = None
Pdolls = SpellDisplay = Aligns = ItemType = None
WeapProfs = CharProfs = RaceData = ClassText = None

Loaded = False

def Load():
	global Classes, KitList, ClassSkills, Races, NextLevel
	global Pdolls, StrModEx, StrMod, SpellDisplay, Aligns
	global ItemType, WeapProfs, CharProfs, RaceData
	global Loaded, ClassText

	if Loaded:
		return

	Classes = GemRB.LoadTable ("classes", False, True)
	ClassSkills = GemRB.LoadTable ("clskills", False, True)
	Races = GemRB.LoadTable ("racetext", False, True)
	if GameCheck.IsIWD2 ():
		RaceData = GemRB.LoadTable ("racetext", False, True)
	else:
		RaceData = GemRB.LoadTable ("racedata", False, True)
	NextLevel = GemRB.LoadTable ("xplevel", False, True)
	StrMod = GemRB.LoadTable ("strmod", False, True)
	StrModEx = GemRB.LoadTable ("strmodex", False, True)
	SpellDisplay = GemRB.LoadTable ("spldisp", False, True)
	ItemType = GemRB.LoadTable ("itemtype", False, True)

	# tables that are only in some games, but not optional there
	if GemRB.HasResource ("kitlist", RES_2DA):
		KitList = GemRB.LoadTable ("kitlist", False, True)
	if GemRB.HasResource ("pdolls", RES_2DA):
		Pdolls = GemRB.LoadTable ("pdolls", False, True)
	if GemRB.HasResource ("aligns", RES_2DA):
		Aligns = GemRB.LoadTable ("aligns", False, True)
	if GemRB.HasResource ("weapprof", RES_2DA):
		WeapProfs = GemRB.LoadTable ("weapprof", False, True)
	if GemRB.HasResource ("charprof", RES_2DA):
		CharProfs = GemRB.LoadTable ("charprof", False, True)
	if GameCheck.IsIWD2 ():
		ClassText = Classes
	elif GemRB.HasResource ("clastext", RES_2DA):
		ClassText = GemRB.LoadTable ("clastext", False, True)

	Loaded = True

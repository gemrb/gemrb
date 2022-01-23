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
# the place for preloading the most commonly used tables
# helps with code deduplication, reduced log spam and tiny lookup savings
import GemRB
from ie_restype import RES_2DA

# these two are only used in SetEncumbranceLabels, but that is called very often
StrMod = StrModEx = None
Classes = KitList = ClassSkills = Races = NextLevel = None
Pdolls = SpellDisplay = Aligns = ItemType = None
WeapProfs = CharProfs = None

Loaded = False

def Load():
	global Classes, KitList, ClassSkills, Races, NextLevel
	global Pdolls, StrModEx, StrMod, SpellDisplay, Aligns
	global ItemType, WeapProfs, CharProfs
	global Loaded

	if Loaded:
		return

	Classes = GemRB.LoadTable ("classes", False, True)
	ClassSkills = GemRB.LoadTable ("clskills", False, True)
	Races = GemRB.LoadTable ("races", False, True)
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

	Loaded = True

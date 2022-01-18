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

	print() # so the following output isn't appended to an existing line
	Classes = GemRB.LoadTable ("classes")
	if GemRB.HasResource ("kitlist", RES_2DA):
		KitList = GemRB.LoadTable ("kitlist")
	ClassSkills = GemRB.LoadTable ("clskills")
	Races = GemRB.LoadTable ("races")
	NextLevel = GemRB.LoadTable ("xplevel")
	if GemRB.HasResource ("pdolls", RES_2DA):
		Pdolls = GemRB.LoadTable ("pdolls")
	StrMod = GemRB.LoadTable ("strmod")
	StrModEx = GemRB.LoadTable ("strmodex")
	SpellDisplay = GemRB.LoadTable ("spldisp")
	if GemRB.HasResource ("aligns", RES_2DA):
		Aligns = GemRB.LoadTable ("aligns")
	ItemType = GemRB.LoadTable ("itemtype")
	if GemRB.HasResource ("weapprof", RES_2DA):
		WeapProfs = GemRB.LoadTable ("weapprof")
	if GemRB.HasResource ("charprof", RES_2DA):
		CharProfs = GemRB.LoadTable ("charprof")

	Loaded = True

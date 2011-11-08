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
import GemRB
from ie_restype import RES_2DA

# these two are only used in SetEncumbranceLabels, but that is called very often
StrMod = StrModEx = None
Classes = KitList = ClassSkills = Races = NextLevel = None
Pdolls = SpellDisplay = None

def Load():
	global Classes, KitList, ClassSkills, Races, NextLevel
	global Pdolls, StrModEx, StrMod, SpellDisplay

	print # so the following output isn't appended to an existing line
	if not Classes:
		Classes = GemRB.LoadTable ("classes")
	if not KitList and GemRB.HasResource("kitlist", RES_2DA):
		KitList = GemRB.LoadTable ("kitlist")
	if not ClassSkills:
		ClassSkills= GemRB.LoadTable ("clskills")
	if not Races:
		Races = GemRB.LoadTable ("races")
	if not NextLevel:
		NextLevel = GemRB.LoadTable ("xplevel")
	if not Pdolls and GemRB.HasResource("pdolls", RES_2DA):
		Pdolls = GemRB.LoadTable ("pdolls")
	if not StrMod:
		StrMod = GemRB.LoadTable ("strmod")
		StrModEx = GemRB.LoadTable ("strmodex")
	if not SpellDisplay:
		SpellDisplay = GemRB.LoadTable ("spldisp")

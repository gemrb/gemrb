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
StrModTable = StrModExTable = None
ClassTable = KitListTable = ClassSkillsTable = RaceTable = NextLevelTable = None
AppearanceAvatarTable = None

def Load():
	global ClassTable, KitListTable, ClassSkillsTable, RaceTable, NextLevelTable
	global AppearanceAvatarTable, StrModExTable, StrModTable

	print # so the following output isn't appended to an existing line
	if not ClassTable:
		ClassTable = GemRB.LoadTable ("classes")
	if not KitListTable and GemRB.HasResource("kitlist", RES_2DA):
		KitListTable = GemRB.LoadTable ("kitlist")
	if not ClassSkillsTable:
		ClassSkillsTable = GemRB.LoadTable ("clskills")
	if not RaceTable:
		RaceTable = GemRB.LoadTable ("races")
	if not NextLevelTable:
		NextLevelTable = GemRB.LoadTable ("xplevel")
	if not AppearanceAvatarTable and GemRB.HasResource("pdolls", RES_2DA):
		AppearanceAvatarTable = GemRB.LoadTable ("pdolls")
	if not StrModTable:
		StrModTable = GemRB.LoadTable ("strmod")
		StrModExTable = GemRB.LoadTable ("strmodex")

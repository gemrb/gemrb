# -*-python-*-
# GemRB - Infinity Engine Emulator
# Copyright (C) 2014 The GemRB Project
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
# GameCheck.py - functions to check GameType

import GemRB
from ie_restype import RES_WMP, RES_ARE

def IsPST ():
	return GemRB.GameType == "pst"

def IsIWD ():
	return GemRB.GameType == "iwd"

def IsHOW ():
	return GemRB.GameType == "how"

def IsIWD1 ():
	return GemRB.GameType == "iwd" or GemRB.GameType == "how"

def IsIWD2 ():
	return GemRB.GameType == "iwd2"

def IsBG1 ():
	return GemRB.GameType == "bg1"

def IsBG2 ():
	return GemRB.GameType == "bg2"

def IsBG2Demo ():
	return ('BG2Demo' in GemRB.__dict__) and (GemRB.BG2Demo == True)

def IsTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP) and GemRB.GetVar("oldgame") == 0

def HasTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP)

def HasHOW ():
	return GemRB.HasResource ("expmap", RES_WMP)

def HasTOTL ():
	return GemRB.HasResource ("ar9700", RES_ARE)

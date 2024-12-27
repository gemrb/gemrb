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

import os

import GemRB
from ie_restype import RES_WMP, RES_ARE, RES_2DA
from GUIDefines import SV_GAMEPATH

MAX_PARTY_SIZE = GemRB.GetVar ("MaxPartySize")

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

def IsBG2OrEE ():
	return GemRB.GameType in ["bg2", "bg2ee"]

def IsBG2EE ():
	return GemRB.GameType == "bg2ee"

def IsBG2Demo ():
	return ('BG2Demo' in GemRB.__dict__) and (GemRB.BG2Demo == True)

def IsGemRBDemo ():
	return GemRB.GameType == "demo"

def IsTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP) and GemRB.GetVar("oldgame") == 0

def HasTOB ():
	return GemRB.HasResource ("worldm25", RES_WMP)

def HasHOW ():
	return GemRB.HasResource ("expmap", RES_WMP)

def HasTOTL ():
	return GemRB.HasResource ("ar9700", RES_ARE)

def HasBGT ():
	return GemRB.HasResource ("ar7200", RES_ARE)

def HasTutu ():
	return GemRB.HasResource ("fw0125", RES_ARE)

def HasTOTSC ():
	return GemRB.HasResource ("toscst", RES_2DA)

# there are no marker files, so check weidu.log
def HasWideScreenMod ():
	gamePath = GemRB.GetSystemVariable (SV_GAMEPATH)
	weiduLogPath = os.path.join (gamePath, "weidu.log")
	try:
		# latin-1 accepts all 1B codes and we only check the ASCII subset;
		# the log file appears to be inconsistent in some cases
		weiduLog = open (weiduLogPath, encoding='latin-1')
	except OSError:
		return False

	ret = False
	for line in weiduLog:
		if "~WIDESCREEN/" in line and not "Uninstalled" in line:
			ret = True
			break

	weiduLog.close ()
	return ret

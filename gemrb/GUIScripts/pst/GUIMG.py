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
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIMG.py,v 1.1 2004/01/11 16:49:09 edheldil Exp $


# GUIMG.py - scripts to control mage spells windows from GUIMG winpack

###################################################

import GemRB
from GUIDefines import *

from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
import GUICommonWindows

MainWindow = 0

def OpenMageWindow ():
	global MainWindow

	CloseCommonWindows ()
	GemRB.LoadWindowPack ("GUIMG")
        OpenCommonWindows ()
	#MainWindow = GemRB.LoadWindow (3)
	MainWindow = Window = GUICommonWindows.MainWindow
	
	#GemRB.SetVisible (MainWindow, 1)

###################################################
# End of file GUIMG.py

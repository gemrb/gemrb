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
# $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GUIScripts/pst/GUIINV.py,v 1.1 2004/01/11 16:49:09 edheldil Exp $


# GUIINV.py - scripts to control inventory windows from GUIINV winpack

###################################################

import GemRB
from GUICommonWindows import OpenCommonWindows, CloseCommonWindows
import GUICommonWindows

def OpenInventoryWindow ():
	global MainWindow

	CloseCommonWindows ()
	
	GemRB.LoadWindowPack ("GUIINV")
        OpenCommonWindows ()
	MainWindow = Window = GUICommonWindows.MainWindow
	
	#GemRB.SetVisible (MainWindow, 1)

def CloseInventoryWindow ():
	CloseCommonWindows ()

###################################################
# End of file GUIINV.py

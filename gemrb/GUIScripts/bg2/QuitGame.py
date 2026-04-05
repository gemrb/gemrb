# SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# QuitGame.py - display EndGame sequence

###################################################

import GemRB

def OnLoad ():
	GemRB.QuitGame ()
	which = GemRB.GetVar ("QuitGame3")
	if which==-1:
		GemRB.SetNextScript("DemoEnd")
	else:
		GemRB.SetNextScript("Start")
	return

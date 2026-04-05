# SPDX-FileCopyrightText: 2007 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# QuitGame.py - display EndGame sequence

###################################################

import GemRB

def OnLoad ():
	GemRB.QuitGame ()
	GemRB.SetNextScript("Start")

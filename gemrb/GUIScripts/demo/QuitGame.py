# SPDX-FileCopyrightText: 2021 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# QuitGame.py - display EndGame sequence

###################################################

import GemRB

def OnLoad ():
	# the demo currently displays a goodbye manually, so just quit
	GemRB.QuitGame ()
	# make sure to dump to main menu, even in development mode
	GemRB.SetVar ("SkipIntroVideos", 0)
	GemRB.SetNextScript ("Start")

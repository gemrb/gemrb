# SPDX-FileCopyrightText: 2009 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

if(HAVE_LDEXPF EQUAL 1)
ADD_GEMRB_PLUGIN ( BIKPlayer BIKPlayer.cpp dct.cpp fft.cpp GetBitContext.cpp mem.cpp rational.cpp rdft.cpp )
endif()

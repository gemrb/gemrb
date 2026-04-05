# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

IF(PNG_FOUND)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS})
ADD_GEMRB_PLUGIN (PNGImporter PNGImporter.cpp)

TARGET_LINK_LIBRARIES( PNGImporter ${PNG_LIBRARIES} Threads::Threads )
ENDIF( PNG_FOUND )

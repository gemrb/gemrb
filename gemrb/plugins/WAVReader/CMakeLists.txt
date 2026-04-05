# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

FILE( GLOB WAVReader_files *.cpp )

ADD_GEMRB_PLUGIN (WAVReader ${WAVReader_files})

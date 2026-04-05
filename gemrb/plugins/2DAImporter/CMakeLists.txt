# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

ADD_GEMRB_PLUGIN(2DAImporter 2DAImporter.cpp 2DAImporter.h)

ADD_GEMRB_PLUGIN_TEST(2DAImporter
  2DAImporter.cpp
  ../../tests/2DAImporter/Test_2DAImporter.cpp
)

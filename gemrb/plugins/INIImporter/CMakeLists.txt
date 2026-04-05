# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

ADD_GEMRB_PLUGIN(INIImporter INIImporter.cpp)

ADD_GEMRB_PLUGIN_TEST(INIImporter
  INIImporter.cpp
  ../../tests/INIImporter/Test_INIImporter.cpp
)

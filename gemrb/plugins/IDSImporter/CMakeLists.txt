# SPDX-FileCopyrightText: 2008 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

ADD_GEMRB_PLUGIN (IDSImporter IDSImporter.cpp)

ADD_GEMRB_PLUGIN_TEST(IDSImporter
  IDSImporter.cpp
  ../../tests/IDSImporter/Test_IDSImporter.cpp
)

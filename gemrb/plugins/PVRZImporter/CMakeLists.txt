# SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
#
# SPDX-License-Identifier: GPL-2.0-or-later

ADD_GEMRB_PLUGIN (PVRZImporter PVRZImporter.cpp)

ADD_GEMRB_PLUGIN_TEST(PVRZImporter
  PVRZImporter.cpp
  ../../tests/PVRZImporter/Test_PVRZImporter.cpp
)

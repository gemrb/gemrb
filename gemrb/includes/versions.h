// Copyright (C) 2026 The GemRB Project
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef VERSIONS_H
#define VERSIONS_H

enum class GAMVersion {
	GemRB = 0,
	BG = 10,
	IWD = 11,
	PST = 12,
	BG2 = 20, // SoA
	TOB = 21,
	IWD2 = 22
};

enum class CREVersion {
	GemRB = 0,
	V1_0 = 10, // bg1
	V1_1 = 11, // bg2 (still V1.0)
	V1_2 = 12, // pst
	V2_2 = 22, // iwd2
	V9_0 = 90 // iwd
};

#endif

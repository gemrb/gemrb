/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <set>

#include <gtest/gtest.h>

#include "System/VFS.h"

namespace GemRB {

TEST(Test_DirectoryIterator, FileIteration) {
	path_t scanDir = PathJoin("tests", "resources", "VFS", "encoding");
	auto unit = DirectoryIterator{scanDir};
	auto fileList = std::set<path_t>{};

	while (unit) {
		fileList.insert(unit.GetName());
		++unit;
	}

	EXPECT_EQ(fileList.size(), 2);
	EXPECT_TRUE(fileList.find("file.txt") != fileList.cend());
	EXPECT_TRUE(fileList.find("file_äöü.txt") != fileList.cend());
}

TEST(Test_VFS, FileExists) {
#ifdef WIN32
	// TODO: do not use PathJoin here for now, that's currently broken on Win
	EXPECT_TRUE(FileExists("tests\\resources\\VFS\\encoding\\file.txt"));
	EXPECT_TRUE(FileExists("tests\\resources\\VFS\\encoding\\FILE.TXT"));
	EXPECT_TRUE(FileExists("tests\\resources\\VFS\\encoding\\file_äöü.txt"));
	EXPECT_TRUE(FileExists("tests\\resources\\VFS\\encoding\\FILE_ÄÖÜ.TXT"));
	EXPECT_FALSE(FileExists("tests\\resources\\VFS\\encoding\\na"));
#else
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE.TXT")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file_äöü.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE_ÄÖÜ.TXT")));
	EXPECT_FALSE(FileExists(PathJoin(baseDir, "na")));
#endif
}

TEST(VFS_Test, PathAppend_PlainPath) {
	path_t path{"dir"};
	PathAppend(path, "subdir");
#ifdef WIN32
	EXPECT_EQ(path, "dir\\subdir");
#else
	EXPECT_EQ(path, "dir/subdir");
#endif
}

TEST(VFS_Test, PathAppend_PlainPathAndLeadingBackslash) {
	path_t path{"dir"};
	// sample from IWD TotL
	PathAppend(path, "\\data\\zcMHar.bif");
#ifdef WIN32
	EXPECT_EQ(path, "dir\\data\\zcMHar.bif");
#else
	EXPECT_EQ(path, "dir/data\\zcMHar.bif");
#endif
}

TEST(VFS_Test, PathAppend_SuffixedPath) {
#ifdef WIN32
	path_t path{"dir\\"};
	PathAppend(path, "subdir");
	EXPECT_EQ(path, "dir\\subdir");
#else
	path_t path{"dir/"};
	PathAppend(path, "subdir");
	EXPECT_EQ(path, "dir/subdir");
#endif
}

}

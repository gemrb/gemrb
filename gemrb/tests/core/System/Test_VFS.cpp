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

#define IS_CASE_INSENSITIVE (WIN32 || __APPLE__)

namespace GemRB {

TEST(DirectoryIterator_Test, DirectoryIteration) {
	path_t scanDir = PathJoin("tests", "resources", "VFS", "encoding");
	auto unit = DirectoryIterator{scanDir};
	unit.SetFlags(DirectoryIterator::Directories, true);

	auto dirList = std::set<path_t>{};

	while (unit) {
		dirList.insert(unit.GetName());
		EXPECT_TRUE(unit.IsDirectory());
		++unit;
	}

	EXPECT_EQ(dirList.size(), 2);
	EXPECT_TRUE(dirList.find("directory") != dirList.cend());
	EXPECT_TRUE(dirList.find("directory_äöü") != dirList.cend());
}

TEST(DirectoryIterator_Test, FileIteration) {
	path_t scanDir = PathJoin("tests", "resources", "VFS", "encoding");
	auto unit = DirectoryIterator{scanDir};
	unit.SetFlags(DirectoryIterator::Files, true);

	auto fileList = std::set<path_t>{};

	while (unit) {
		fileList.insert(unit.GetName());
		++unit;
	}

	EXPECT_EQ(fileList.size(), 2);
	EXPECT_TRUE(fileList.find("file.txt") != fileList.cend());
	EXPECT_TRUE(fileList.find("file_äöü.txt") != fileList.cend());
}

TEST(VFS_Test, Misc) {
#ifdef WIN32
	EXPECT_EQ(SPathDelimiter[0], '\\');
#else
	EXPECT_EQ(SPathDelimiter[0], '/');
#endif
}

TEST(VFS_Test, DirExists) {
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "directory")));
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "directory_äöü")));
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "Directory_ÄÖÜ")));
	EXPECT_FALSE(DirExists(PathJoin(baseDir, "does_not_exist")));
}

TEST(VFS_Test, FileExists) {
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE.TXT")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file_äöü.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE_ÄÖÜ.TXT")));
	EXPECT_FALSE(FileExists(PathJoin(baseDir, "na")));
}

TEST(VFS_Test, PathAppend_PlainPath) {
	path_t path{"dir"};
	PathAppend(path, "subdir");
	EXPECT_EQ(path, fmt::format("dir{0}subdir", SPathDelimiter));
}

TEST(VFS_Test, PathAppend_SuffixedPath) {
	path_t path = fmt::format("dir{0}", SPathDelimiter);
	PathAppend(path, "subdir");
	EXPECT_EQ(path, fmt::format("dir{0}subdir", SPathDelimiter));
}

TEST(VFS_Test, PathJoin) {
	EXPECT_EQ(PathJoin("dir", "abc"), PathJoin<true>("dir", "abc"));
	EXPECT_EQ(PathJoin("dir", "abc"), fmt::format("dir{0}abc", SPathDelimiter));
}

TEST(VFS_Test, PathJoin_WithActualCaseFix) {
	auto path = PathJoin<true>("tests", "resoUrces", "vfs", "encoding", "file_ÄÖÜ.txt");
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.txt", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

TEST(VFS_Test, PathJoin_WithoutActualCaseFix) {
	auto path = PathJoin<false>("tests", "resoUrces", "vfs");
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs", SPathDelimiter));
}

TEST(VFS_Test, PathJoinExt) {
	EXPECT_EQ(PathJoinExt("dir", "file", "ext"), PathJoinExt<true>("dir", "file", "ext"));
	EXPECT_EQ(PathJoinExt("dir", "file", "ext"), fmt::format("dir{0}file.ext", SPathDelimiter));
}

TEST(VFS_Test, PathJoinExt_WithActualCaseFix) {
	auto basePath = PathJoin<false>("tests", "resoUrces", "vfs", "encoding");
	auto path = PathJoinExt<true>(basePath, "file_ÄÖÜ", "TXT");
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.TXT", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

TEST(VFS_Test, PathJoinExt_WithoutActualCaseFix) {
	auto basePath = PathJoin<false>("tests", "resoUrces", "vfs");
	auto path = PathJoinExt<false>(basePath, "file_ÄÖÜ", "TXT");
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}file_ÄÖÜ.TXT", SPathDelimiter));
}

TEST(VFS_Test, ResolveCase) {
	auto badPath = PathJoin<false>("tests", "resoUrces", "vfs", "encoding", "file_ÄÖÜ.txt");
	const auto path = ResolveCase(badPath);
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.txt", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

}

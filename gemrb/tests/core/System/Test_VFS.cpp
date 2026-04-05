// SPDX-FileCopyrightText: 2023 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Streams/FileStream.h"
#include "Strings/StringConversion.h"
#include "System/VFS.h"

#include <gtest/gtest.h>
#include <set>

#define IS_CASE_INSENSITIVE (WIN32 || __APPLE__)

namespace GemRB {

path_t getTempPath();

TEST(DirectoryIteratorTest, DirectoryIteration)
{
	path_t scanDir = PathJoin("tests", "resources", "VFS", "encoding");
	auto unit = DirectoryIterator { scanDir };
	unit.SetFlags(DirectoryIterator::Directories, true);

	auto dirList = std::set<path_t> {};

	while (unit) {
		dirList.insert(unit.GetName());
		EXPECT_TRUE(unit.IsDirectory());
		++unit;
	}

	EXPECT_EQ(dirList.size(), size_t(2));
	EXPECT_TRUE(dirList.find("directory") != dirList.cend());
	EXPECT_TRUE(dirList.find("directory_äöü") != dirList.cend());
}

TEST(DirectoryIteratorTest, FileIteration)
{
	path_t scanDir = PathJoin("tests", "resources", "VFS", "encoding");
	auto unit = DirectoryIterator { scanDir };
	unit.SetFlags(DirectoryIterator::Files, true);

	auto fileList = std::set<path_t> {};

	while (unit) {
		fileList.insert(unit.GetName());
		++unit;
	}

	EXPECT_EQ(fileList.size(), size_t(2));
	EXPECT_TRUE(fileList.find("file.txt") != fileList.cend());
	EXPECT_TRUE(fileList.find("file_äöü.txt") != fileList.cend());
}

TEST(VFSTest, Misc)
{
#ifdef WIN32
	EXPECT_EQ(SPathDelimiter[0], '\\');
#else
	EXPECT_EQ(SPathDelimiter[0], '/');
#endif
}

TEST(VFSTest, DirExists)
{
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "directory")));
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "directory_äöü")));
	EXPECT_TRUE(DirExists(PathJoin(baseDir, "Directory_ÄÖÜ")));
	EXPECT_FALSE(DirExists(PathJoin(baseDir, "does_not_exist")));
}

TEST(VFSTest, FileExists)
{
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE.TXT")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "file_äöü.txt")));
	EXPECT_TRUE(FileExists(PathJoin(baseDir, "FILE_ÄÖÜ.TXT")));
	EXPECT_FALSE(FileExists(PathJoin(baseDir, "na")));
}

TEST(VFSTest, PathAppendPlainPath)
{
	path_t path { "dir" };
	PathAppend(path, "subdir");
	EXPECT_EQ(path, fmt::format("dir{0}subdir", SPathDelimiter));
}

TEST(VFSTest, PathAppendSuffixedPath)
{
	path_t path = fmt::format("dir{0}", SPathDelimiter);
	PathAppend(path, "subdir");
	EXPECT_EQ(path, fmt::format("dir{0}subdir", SPathDelimiter));
}

TEST(VFSTest, PathJoin)
{
	EXPECT_EQ(PathJoin("dir", "abc"), PathJoin<true>("dir", "abc"));
	EXPECT_EQ(PathJoin("dir", "abc"), fmt::format("dir{0}abc", SPathDelimiter));
}

TEST(VFSTest, PathJoinWithActualCaseFix)
{
	auto path = PathJoin<true>("tests", "resoUrces", "vfs", "encoding", "file_ÄÖÜ.txt");
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.txt", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

TEST(VFSTest, PathJoinWithoutActualCaseFix)
{
	auto path = PathJoin<false>("tests", "resoUrces", "vfs");
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs", SPathDelimiter));
}

TEST(VFSTest, PathJoinExt)
{
	EXPECT_EQ(PathJoinExt("dir", "file", "ext"), PathJoinExt<true>("dir", "file", "ext"));
	EXPECT_EQ(PathJoinExt("dir", "file", "ext"), fmt::format("dir{0}file.ext", SPathDelimiter));
}

TEST(VFSTest, PathJoinExtWithActualCaseFix)
{
	auto basePath = PathJoin<false>("tests", "resoUrces", "vfs", "encoding");
	auto path = PathJoinExt<true>(basePath, "file_ÄÖÜ", "TXT");
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.TXT", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

TEST(VFSTest, PathJoinExtWithoutActualCaseFix)
{
	auto basePath = PathJoin<false>("tests", "resoUrces", "vfs");
	auto path = PathJoinExt<false>(basePath, "file_ÄÖÜ", "TXT");
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}file_ÄÖÜ.TXT", SPathDelimiter));
}

TEST(VFSTest, ResolveCase)
{
	auto badPath = PathJoin<false>("tests", "resoUrces", "vfs", "encoding", "file_ÄÖÜ.txt");
	const auto& path = ResolveCase(badPath);
#if IS_CASE_INSENSITIVE
	EXPECT_EQ(path, fmt::format("tests{0}resoUrces{0}vfs{0}encoding{0}file_ÄÖÜ.txt", SPathDelimiter));
#else
	EXPECT_EQ(path, "tests/resources/VFS/encoding/file_äöü.txt");
#endif
}

TEST(VFSTest, MakeDirectory)
{
	auto tempPath = getTempPath();

	auto newDirectoryPath = PathJoin(tempPath, "new_directory");
	EXPECT_TRUE(MakeDirectory(newDirectoryPath));
	EXPECT_TRUE(DirExists(newDirectoryPath));
	RemoveDirectory(newDirectoryPath);

	auto umlautPath = PathJoin(tempPath, "test_ÖÄü");
	EXPECT_TRUE(MakeDirectory(umlautPath));
	EXPECT_TRUE(DirExists(umlautPath));
	RemoveDirectory(umlautPath);
}

TEST(VFSTest, MakeDirectories)
{
	auto tempPath = getTempPath();

	auto newDirectoryPath = PathJoin(tempPath, "test", "new");
	EXPECT_TRUE(MakeDirectories(newDirectoryPath));
	EXPECT_TRUE(DirExists(newDirectoryPath));
	RemoveDirectory(newDirectoryPath);
	RemoveDirectory(PathJoin(tempPath, "test"));

	auto umlautPath = PathJoin(tempPath, "test_Ö", "ü");
	EXPECT_TRUE(MakeDirectories(umlautPath));
	EXPECT_TRUE(DirExists(umlautPath));
	RemoveDirectory(newDirectoryPath);
	RemoveDirectory(PathJoin(tempPath, "test_Ö"));
}

TEST(VFSTest, RemoveDirectory)
{
	auto tempPath = getTempPath();

	auto newDirectoryPath = PathJoin(tempPath, "test", "new2");
	EXPECT_TRUE(MakeDirectories(newDirectoryPath));
	EXPECT_TRUE(RemoveDirectory(newDirectoryPath));
	EXPECT_FALSE(DirExists(newDirectoryPath));
	RemoveDirectory(PathJoin(tempPath, "test"));

	auto umlautDirectoryPath = PathJoin(tempPath, "test_Ö", "ü");
	EXPECT_TRUE(MakeDirectories(umlautDirectoryPath));
	EXPECT_TRUE(RemoveDirectory(umlautDirectoryPath));
	EXPECT_FALSE(DirExists(umlautDirectoryPath));
	RemoveDirectory(PathJoin(tempPath, "test_Ö"));
}

TEST(VFSTest, UnlinkFile)
{
	auto tempPath = getTempPath();

	auto filePath = PathJoin(tempPath, "test.txt");
	{
		FileT file;
		file.OpenNew(filePath);
	}
	EXPECT_TRUE(UnlinkFile(filePath));
	EXPECT_FALSE(FileExists(filePath));

	auto umlautFilePath = PathJoin(tempPath, "test_äß.txt");
	{
		FileT file;
		file.OpenNew(umlautFilePath);
	}
	EXPECT_TRUE(UnlinkFile(umlautFilePath));
	EXPECT_FALSE(FileExists(umlautFilePath));
}

TEST(VFSTest, FileModificationTime)
{
	auto baseDir = PathJoin("tests", "resources", "VFS", "encoding");
	EXPECT_NE(nullptr, FileModificationTime(PathJoin(baseDir, "file.txt")));
	EXPECT_NE(nullptr, FileModificationTime(PathJoin(baseDir, "file_äöü.txt")));
	EXPECT_EQ(nullptr, FileModificationTime("na"));
}

path_t getTempPath()
{
#ifdef WIN32
	constexpr DWORD pathLength = MAX_PATH + 1;
	WCHAR tempPath[pathLength] = L"";

	DWORD length = GetTempPath(pathLength, tempPath);
	if (length == 0) {
		assert(false);
	}

	auto result = RecodedStringFromWideStringBytes(reinterpret_cast<const char16_t*>(tempPath), length * 2, "UTF-8");
	result.resize(length);

	return result;
#else
	char* tmpDir = getenv("TMPDIR");
	if (tmpDir == nullptr) {
		tmpDir = getenv("TMP");
	}
	if (tmpDir == nullptr) {
		return "/tmp";
	}

	return tmpDir;
#endif
}

}

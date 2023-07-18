/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include <psp2/kernel/iofilemgr.h>

#define S_IREAD SCE_S_IRUSR
#define S_IWRITE SCE_S_IWUSR
#define S_IEXEC SCE_S_IXUSR

struct DIR
{
	DIR() : is_first(true) {}

	bool is_first;
	SceUID descriptor;
};

struct dirent
{
	dirent() : d_name(nullptr) {}

	void assign(std::string&& value) {
		buffer = std::move(value);
		d_name = buffer.data();
	}

	std::string buffer;
	char *d_name;
};

inline DIR* opendir(const char *filename)
{
	auto *dir = new DIR{};
	dir->descriptor = sceIoDopen(filename);

	if (dir->descriptor <= 0) {
		delete dir;
		return nullptr;
	}

	return dir;
}

inline dirent* readdir(DIR *dir)
{
	static dirent de;
	//vitasdk kind of skips current directory entry..
	if (dir->is_first) {
		dir->is_first = false;
		de.assign(".");
	} else {
		SceIoDirent entry;
		if (sceIoDread(dir->descriptor, &entry) <= 0)
			return nullptr;
		de.assign(std::string(entry.d_name, _MAX_PATH));
	}

	return &de;
}

inline void closedir(DIR *dir)
{
	sceIoDclose(dir->descriptor);
	delete dir;
}

inline int mkdir(const char *path, SceMode mode)
{
	return sceIoMkdir(path, mode);
}

inline int rmdir(const char *path)
{
	return sceIoRemove(path);
}

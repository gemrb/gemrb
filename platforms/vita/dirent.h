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
	bool is_first;
	SceUID descriptor;
};

struct dirent
{
	char d_name[_MAX_PATH];
};

inline DIR* opendir(const char *filename)
{
	DIR *dirp = (DIR*) malloc(sizeof(DIR));
	dirp->is_first = 1;
	dirp->descriptor = sceIoDopen(filename);

	if (dirp->descriptor <= 0) {
		free(dirp);
		return NULL;
	}

	return dirp;
}

inline dirent* readdir(DIR *dirp)
{
	static dirent de;
	//vitasdk kind of skips current directory entry..
	if (dirp->is_first) {
		dirp->is_first = 0;
		strncpy(de.d_name, ".", 1);
	} else {
		SceIoDirent dir;
		if (sceIoDread(dirp->descriptor, &dir) <= 0)
			return NULL;
		strncpy(de.d_name, dir.d_name, 256);
	}

	return &de;
}

inline void closedir(DIR *dirp)
{
	sceIoDclose(dirp->descriptor);
	free(dirp);
}

inline int mkdir(const char *path, SceMode mode)
{
	return sceIoMkdir(path, mode);
}

inline int rmdir(const char *path)
{
	return sceIoRemove(path);
}

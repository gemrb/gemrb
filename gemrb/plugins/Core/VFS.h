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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/VFS.h,v 1.3 2004/02/18 14:04:04 edheldil Exp $
 *
 */

#ifndef VFS_H
#define VFS_H

#include "../../includes/globals.h"

#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <dlfcn.h>
#endif

#ifdef WIN32
#define _MAX_PATH 260
#else
#define _MAX_PATH FILENAME_MAX
#endif

#ifndef S_ISDIR
#define S_ISDIR(x) (x & S_IFDIR)
#endif

#ifdef WIN32

typedef struct DIR {
  char path[_MAX_PATH];
  bool is_first;
  struct _finddata_t c_file;
  long hFile;
} DIR;

typedef struct dirent {
  char d_name[_MAX_PATH];
} dirent;

DIR * opendir (const char *filename);
dirent * readdir (DIR *dirp);
void closedir (DIR *dirp);
#endif

#endif  // !VFS_H

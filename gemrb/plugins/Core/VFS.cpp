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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/VFS.cpp,v 1.1 2004/02/18 11:14:19 balrog994 Exp $
 *
 */

// VFS.cpp : functions to access filesystem in os-independent way
//           and POSIX-like compatibility layer for win

#include "VFS.h"

#ifdef WIN32

// buffer which readdir returns
static dirent de;


DIR * opendir (const char *filename)
{
  DIR *dirp = (DIR*)malloc (sizeof (DIR));
  dirp->is_first = 1;

  sprintf(dirp->path, "%s%s*.*", filename, SPathDelimiter);
  //if((hFile = (long)_findfirst(Path, &c_file)) == -1L) //If there is no file matching our search

  return dirp;
}

dirent * readdir (DIR *dirp)
{
  struct _finddata_t c_file;
  
  if (dirp->is_first) {
    dirp->is_first = 0;
    dirp->hFile = (long)_findfirst(dirp->path, &c_file);
    if (dirp->hFile == -1L)
      return NULL;
  } else {
    if ((long)_findnext(dirp->hFile, &c_file) != 0)
      return NULL;
  }

  strcpy (de.d_name, c_file.name);

  return &de;
}

void closedir (DIR *dirp)
{
  _findclose (dirp->hFile);
  free (dirp);
}


/*int stat (const char *path, struct stat *sb)
{
	struct _finddata_t c_file;

    long hFile = (long)_findfirst (path, &c_file);
    if (hFile == -1L) 
      return -1;

    

}*/

#endif  // WIN32

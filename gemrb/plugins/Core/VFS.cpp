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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/VFS.cpp,v 1.2 2004/02/18 15:04:21 balrog994 Exp $
 *
 */

// VFS.cpp : functions to access filesystem in os-independent way
//           and POSIX-like compatibility layer for win

#include "../../includes/globals.h"
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

#endif  // WIN32

_FILE * _fopen(const char * filename, const char * mode)
{
#ifdef WIN32
	DWORD OpenFlags;
	DWORD AccessFlags;
	while(*mode) {
		if((*mode == 'w') || (*mode == 'W')) {
			OpenFlags |= OPEN_ALWAYS;
			AccessFlags |= GENERIC_WRITE;
		} else if((*mode == 'r') || (*mode == 'R')) {
			OpenFlags |= OPEN_EXISTING;
			AccessFlags |= GENERIC_READ;
		} else if((*mode == 'a') || (*mode == 'A')) {
			OpenFlags |= OPEN_ALWAYS;
			AccessFlags |= GENERIC_READ | GENERIC_WRITE;
		} else if(*mode == '+') {
			AccessFlags |= GENERIC_READ | GENERIC_WRITE;
		}
		mode++;
	}
	HANDLE hFile = CreateFile(filename, 
							GENERIC_READ, 
							FILE_SHARE_READ, 
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return NULL;
	_FILE * ret = (_FILE*)malloc(sizeof(_FILE));
	ret->hFile = hFile;
	return ret;
#else
	return fopen(filename, mode);
#endif
}

size_t _fread(void * ptr, size_t size, size_t n, _FILE * stream)
{
#ifdef WIN32
	if(!stream)
		return (size_t)0;
	unsigned long read;
	if(!ReadFile(stream->hFile,
			ptr,
			(unsigned long)(size*n),
			&read,
			NULL))
		return (size_t)0;
	return (size_t)read;
#else
	return fread(ptr, size, n, stream);
#endif
}

size_t _fwrite(const void *ptr, size_t size, size_t n, _FILE *stream)
{
#ifdef WIN32
	if(!stream)
		return (size_t)0;
	unsigned long wrote;
	if(!WriteFile(stream->hFile,
			ptr,
			(unsigned long)(size*n),
			&wrote,
			NULL))
		return (size_t)0;
	return (size_t)wrote;
#else
	return fwrite(ptr, size, n, stream);
#endif	
}

size_t _fseek(_FILE * stream, long offset, int whence)
{
#ifdef WIN32
	if(!stream)
		return (size_t)1;
	unsigned long method;
	switch(whence) {
		case SEEK_SET:
			method = FILE_BEGIN;
		break;
		case SEEK_CUR:
			method = FILE_CURRENT;
		break;
		case SEEK_END:
			method = FILE_END;
		break;
	}
	if(SetFilePointer(stream->hFile,
			offset,
			NULL,
			method) == 0xffffffff)
		return (size_t)1;
	return (size_t)0;
#else
	return fwrite(ptr, size, n, stream);
#endif
}

int _fgetc(_FILE * stream)
{
#ifdef WIN32
	if(!stream)
		return 0;
	unsigned char tmp;
	unsigned long read;
	BOOL bResult = ReadFile(stream->hFile,
							&tmp,
							(unsigned long)1,
							&read,
							NULL);
	if(bResult && read)
		return (int)tmp; 
	return EOF;
#else
	return fgetc(stream);
#endif
}

long int _ftell(_FILE * stream)
{
#ifdef WIN32
	if(!stream)
		return EOF;
	unsigned long pos = SetFilePointer(stream->hFile,
			0,
			NULL,
			FILE_CURRENT);
	if(pos == 0xffffffff)
		return -1L;
	return (long int)pos;
#else
	return ftell(stream);
#endif
}

int _feof(_FILE * stream)
{
#ifdef WIN32
	if(!stream)
		return 0;
	unsigned char tmp;
	unsigned long read;
	BOOL bResult = ReadFile(stream->hFile,
							&tmp,
							(unsigned long)1,
							&read,
							NULL);
	if(bResult && (read == 0))
		return 1; //EOF
	bResult = SetFilePointer(stream->hFile,
			-1,
			NULL,
			FILE_CURRENT);
	return 0;
#else
	return feof(stream);
#endif
}

int _fclose(_FILE * stream)
{
#ifdef WIN32
	if(!stream)
		return EOF;
	if(!CloseHandle(stream->hFile))
		return EOF;
	free(stream);
	return 0;
#else
	return fclose(stream);
#endif
}



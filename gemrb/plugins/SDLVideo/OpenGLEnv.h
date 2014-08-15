/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2014 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef GemRB_OpenGLEnv_h
#define GemRB_OpenGLEnv_h

#if USE_GL
	#include <GL/glew.h>
#else
	#if __APPLE__
		#include <OpenGLES/ES2/gl.h>
		#include <OpenGLES/ES2/glext.h>
	#else
		#include <GLES2/GL2.h>
		#include <GLES2/GL2ext.h>
	#endif
#endif

#endif

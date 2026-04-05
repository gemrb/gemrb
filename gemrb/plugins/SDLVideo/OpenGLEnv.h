// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef GemRB_OpenGLEnv_h
#define GemRB_OpenGLEnv_h

#define GL_GLEXT_PROTOTYPES 1

#ifdef __APPLE__
	#include <OpenGL/gl.h>
#else
	#ifdef USE_OPENGL_API
		#ifdef _WIN32
			#include <GL/glew.h>
		#else
			#include <SDL_opengl.h>
		#endif
	#else // USE_GLES_API
		#include <SDL_opengles2.h>
	#endif
#endif

#endif

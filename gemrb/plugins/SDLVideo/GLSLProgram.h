#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif
#include <map>

namespace GemRB 
{
	class GLSLProgram
	{
	public:
		static GLSLProgram* Create(const char* vertexSource, const char* fragmentSource);
		static void GetLastError(char* msg , unsigned int msgSize);

		void Release();
		void Use();
		void UnUse();
		bool StoreUniformLocation(const char* uniformName);
		bool SetUniformValue(const char* uniformName, const unsigned char size, GLfloat value1, GLfloat value2 = 0.0f, GLfloat value3 = 0.0f, GLfloat value4 = 0.0f);
		bool SetUniformValue(const char* uniformName, const unsigned char size, GLint value1, GLint value2 = 0, GLint value3 = 0, GLint value4 = 0);
		bool SetUniformValue(const char* uniformName, const unsigned char size, GLsizei count, const GLfloat* value);
		bool SetUniformValue(const char* uniformName, const unsigned char size, GLsizei count, const GLint* value);
		bool SetUniformMatrixValue(const char* uniformName, const unsigned char size, GLsizei count, GLboolean transpose, const GLfloat* value);
		GLint GetAttribLocation(const char* attribName);
	private:	
		static char errMessage[512];

		GLuint program;
		std::map<const char*, GLint> uniforms;
		bool buildProgram(const char* vertexSource, const char* fragmentSource);
		GLuint buildShader(GLenum type, const char* source);
		GLint getUniformLocation(const char* uniformName);
		GLSLProgram(){}
		~GLSLProgram(){}
	};
};

#endif
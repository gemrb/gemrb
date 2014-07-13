#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif
#include <map>
#include <string>

namespace GemRB 
{
	class GLSLProgram
	{
	public:
		static GLSLProgram* Create(std::string vertexSource, std::string fragmentSource);
		static GLSLProgram* CreateFromFiles(std::string vertexSourceFileName, std::string fragmentSourceFileName);

		static const std::string GetLastError();

		void Release();
		void Use();
		void UnUse();
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLfloat value1, GLfloat value2 = 0.0f, GLfloat value3 = 0.0f, GLfloat value4 = 0.0f);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLint value1, GLint value2 = 0, GLint value3 = 0, GLint value4 = 0);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLint* value);
		bool SetUniformMatrixValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value);
		GLint GetAttribLocation(std::string attribName);
	private:	
		static std::string errMessage;

		GLuint program;
		std::map<std::string, GLint> uniforms;
		bool buildProgram(std::string vertexSource, std::string fragmentSource);
		GLuint buildShader(GLenum type, std::string source);
		GLint getUniformLocation(std::string uniformName);
		bool storeUniformLocation(std::string uniformName);
		GLSLProgram(){}
		~GLSLProgram(){}
	};
}

#endif
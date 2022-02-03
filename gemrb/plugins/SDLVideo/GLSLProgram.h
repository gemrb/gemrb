#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include "OpenGLEnv.h"

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

		~GLSLProgram();
		void Use() const;
		void UnUse() const;
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLfloat value1, GLfloat value2 = 0.0f, GLfloat value3 = 0.0f, GLfloat value4 = 0.0f);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLint value1, GLint value2 = 0, GLint value3 = 0, GLint value4 = 0);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value);
		bool SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLint* value);
		bool SetUniformMatrixValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value);
		GLint GetAttribLocation(std::string attribName) const;
	private:	
		static std::string errMessage;

		GLuint program;
		std::map<std::string, GLint> uniforms;
		bool buildProgram(std::string vertexSource, const std::string& fragmentSource);
		GLuint buildShader(GLenum type, std::string source) const;
		GLint getUniformLocation(const std::string& uniformName) const;
		bool storeUniformLocation(std::string uniformName);
	};
}

#endif

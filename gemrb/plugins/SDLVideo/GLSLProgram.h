#ifndef GLSLPROGRAM_H
#define GLSLPROGRAM_H

#include "OpenGLEnv.h"

#include <map>
#include <string>

namespace GemRB {
class GLSLProgram {
public:
	static GLSLProgram* Create(const std::string& vertexSource, const std::string& fragmentSource, GLuint programID = 0);
	static GLSLProgram* CreateFromFiles(std::string vertexSourceFileName, std::string fragmentSourceFileName, GLuint programID = 0);

	static std::string GetLastError();

	GLSLProgram() = default;
	GLSLProgram(const GLSLProgram&) = delete;
	~GLSLProgram();
	GLSLProgram& operator=(const GLSLProgram&) = delete;
	void Use() const;
	void UnUse() const;
	bool SetUniformValue(const std::string& uniformName, const unsigned char size, GLfloat value1, GLfloat value2 = 0.0f, GLfloat value3 = 0.0f, GLfloat value4 = 0.0f) const;
	bool SetUniformValue(const std::string& uniformName, const unsigned char size, GLint value1, GLint value2 = 0, GLint value3 = 0, GLint value4 = 0) const;
	bool SetUniformValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLfloat* value) const;
	bool SetUniformValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLint* value) const;
	bool SetUniformMatrixValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLfloat* value) const;
	GLint GetAttribLocation(const std::string& attribName) const;
	GLuint GetProgramID() const;

private:
	static std::string errMessage;

	GLuint program = 0;
	std::map<std::string, GLint> uniforms;
	bool buildProgram(const std::string& vertexSource, const std::string& fragmentSource, GLuint programID);
	GLuint buildShader(GLenum type, std::string source) const;
	GLint getUniformLocation(const std::string& uniformName) const;
	bool storeUniformLocation(const std::string& uniformName);
	static bool TryPath(std::ifstream& fileStream, const std::string& shaderPath);
};
}

#endif

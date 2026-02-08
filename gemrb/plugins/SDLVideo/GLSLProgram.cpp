#include "GLSLProgram.h"

#include "System/VFS.h" // for PathDelimiter

#include <cstdio>
#include <fstream>

using namespace GemRB;

std::string GLSLProgram::errMessage;

GLSLProgram::~GLSLProgram()
{
	if (program != 0) glDeleteProgram(program);
}

GLSLProgram* GLSLProgram::Create(const std::string& vertexSource, const std::string& fragmentSource, GLuint programID)
{
	GLSLProgram* program = new GLSLProgram();
	if (!program->buildProgram(vertexSource, fragmentSource, programID)) {
		delete program;
		return NULL;
	}
	return program;
}

bool GLSLProgram::TryPath(std::ifstream& fileStream, std::string& shaderPath)
{
#if __APPLE__
	if (!fileStream.is_open()) {
		path_t bundleShaderPath = BundlePath(RESOURCES);
		PathAppend(bundleShaderPath, shaderPath);
		ResolveFilePath(bundleShaderPath);
		fileStream.open(bundleShaderPath);
	}
#elif defined DATA_DIR
	if (!fileStream.is_open()) {
		shaderPath.insert(0, 1, PathDelimiter);
		shaderPath.insert(0, DATA_DIR);
		fileStream.open(shaderPath);
	}
#endif
	if (!fileStream.is_open()) {
		GLSLProgram::errMessage = "GLSLProgram error: Can't open file: " + shaderPath;
		return false;
	}
	return true;
}

GLSLProgram* GLSLProgram::CreateFromFiles(std::string vertexSourceFileName, std::string fragmentSourceFileName,
					  GLuint programID)
{
	std::string vertexContent;
	std::string fragmentContent;

	// first check the build dir then fallback to DATA_DIR
	std::ifstream fileStream(vertexSourceFileName);
	if (!TryPath(fileStream, vertexSourceFileName)) {
		return nullptr;
	}
	std::string line = "";
	while (!fileStream.eof()) {
		std::getline(fileStream, line);
		line.erase(0, line.find_first_not_of("\n\t\r "));
#if USE_OPENGL_API
		// remove precisions when using OpenGL and not GLES
		if (line.find("precision") == 0) continue;
#endif
		vertexContent.append(line + "\n");
	}
	fileStream.close();

	std::ifstream fileStream2(fragmentSourceFileName);
	if (!TryPath(fileStream2, fragmentSourceFileName)) {
		return nullptr;
	}
	while (!fileStream2.eof()) {
		std::getline(fileStream2, line);
		line.erase(0, line.find_first_not_of("\n\t\r "));
#if USE_OPENGL_API
		// remove precisions
		if (line.find("precision") == 0) continue;
#endif
		fragmentContent.append(line + "\n");
	}
	fileStream2.close();

	return GLSLProgram::Create(vertexContent, fragmentContent, programID);
}

GLuint GLSLProgram::buildShader(GLenum type, std::string shader_source) const
{
#if USE_OPENGL_API
	// for GLSL 1.X (GL 2.0)
	shader_source.insert(0, "#version 110\n");
#else
	// for GLSL ES 1.00 (GLES 2.0)
	shader_source.insert(0, "#version 100\n");
#endif
	GLuint id = glCreateShader(type);
	const char* src = shader_source.c_str();
	glShaderSource(id, 1, &src, 0);
	glCompileShader(id);
	GLint result = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result != GL_TRUE) {
		char buff[512];
		glGetShaderInfoLog(id, sizeof(buff), 0, buff);
		GLSLProgram::errMessage = std::string(buff);
		glDeleteShader(id);
		return 0;
	}
	return id;
}

bool GLSLProgram::buildProgram(const std::string& vertexSource, const std::string& fragmentSource, GLuint programID)
{
	program = programID;

	if (programID == 0) {
		program = glCreateProgram();
		if (program == 0) {
			GLSLProgram::errMessage = "GLSLProgram error: glCreateProgram failed";
			glDeleteProgram(program);
			return false;
		}
	} else {
		GLsizei numAttachedShaders = 0;
		// Assume there is nothing but VS & FS.
		GLuint shaderIDs[2] = { 0 };
		glGetAttachedShaders(program, 2, &numAttachedShaders, shaderIDs);

		for (GLsizei i = 0; i < numAttachedShaders; ++i) {
			glDetachShader(program, shaderIDs[i]);
		}
	}

	GLuint vertexId = buildShader(GL_VERTEX_SHADER, vertexSource);
	if (vertexId == 0) {
		glDeleteProgram(program);
		return false;
	}
	GLuint fragmentId = buildShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (fragmentId == 0) {
		glDeleteProgram(program);
		return false;
	}

	glAttachShader(program, vertexId);
	glAttachShader(program, fragmentId);

	glLinkProgram(program);
	GLint result = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result != GL_TRUE) {
		char buff[512];
		glGetProgramInfoLog(program, sizeof(buff), 0, buff);
		GLSLProgram::errMessage = std::string(buff);
		glDeleteProgram(program);
		program = 0;
	}
	glDeleteShader(vertexId);
	glDeleteShader(fragmentId);
	if (program != 0) {
		int count = -1;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
		for (int i = 0; i < count; i++) {
			int name_len = -1;
			int num = -1;
			GLenum type = GL_ZERO;
			char name[64];
			glGetActiveUniform(program, GLuint(i), sizeof(name) - 1, &name_len, &num, &type, name);
			name[name_len] = 0;
			storeUniformLocation(std::string(name));
		}
	}
	return program != 0;
}

GLint GLSLProgram::getUniformLocation(const std::string& uniformName) const
{
	const auto it = uniforms.find(uniformName);
	if (it == uniforms.end()) {
		GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform location";
		return -1;
	}

	return it->second;
}

std::string GLSLProgram::GetLastError()
{
	return GLSLProgram::errMessage;
}

bool GLSLProgram::storeUniformLocation(const std::string& uniformName)
{
	if (uniforms.find(uniformName) == uniforms.end()) {
		GLint location = glGetUniformLocation(program, uniformName.c_str());
		if (location == -1) {
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform location";
			return false;
		}
		uniforms[uniformName] = location;
		return true;
	} else {
		GLSLProgram::errMessage = "GLSLProgram error: Uniform already stored";
		return true;
	}
}

bool GLSLProgram::SetUniformValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLfloat* value) const
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch (size) {
		case 1:
			glUniform1fv(location, count, value);
			return true;
		case 2:
			glUniform2fv(location, count, value);
			return true;
		case 3:
			glUniform3fv(location, count, value);
			return true;
		case 4:
			glUniform4fv(location, count, value);
			return true;
		default:
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform size";
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLint* value) const
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch (size) {
		case 1:
			glUniform1iv(location, count, value);
			return true;
		case 2:
			glUniform2iv(location, count, value);
			return true;
		case 3:
			glUniform3iv(location, count, value);
			return true;
		case 4:
			glUniform4iv(location, count, value);
			return true;
		default:
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform size";
			return false;
	}
}

bool GLSLProgram::SetUniformMatrixValue(const std::string& uniformName, const unsigned char size, GLsizei count, const GLfloat* value) const
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch (size) {
		case 2:
			glUniformMatrix2fv(location, count, GL_FALSE, value);
			return true;
		case 3:
			glUniformMatrix3fv(location, count, GL_FALSE, value);
			return true;
		case 4:
			glUniformMatrix4fv(location, count, GL_FALSE, value);
			return true;
		default:
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform size";
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const std::string& uniformName, const unsigned char size, GLfloat value1, GLfloat value2, GLfloat value3, GLfloat value4) const
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch (size) {
		case 1:
			glUniform1f(location, value1);
			return true;
		case 2:
			glUniform2f(location, value1, value2);
			return true;
		case 3:
			glUniform3f(location, value1, value2, value3);
			return true;
		case 4:
			glUniform4f(location, value1, value2, value3, value4);
			return true;
		default:
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform size";
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const std::string& uniformName, const unsigned char size, GLint value1, GLint value2, GLint value3, GLint value4) const
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch (size) {
		case 1:
			glUniform1i(location, value1);
			return true;
		case 2:
			glUniform2i(location, value1, value2);
			return true;
		case 3:
			glUniform3i(location, value1, value2, value3);
			return true;
		case 4:
			glUniform4i(location, value1, value2, value3, value4);
			return true;
		default:
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform size";
			return false;
	}
}

void GLSLProgram::Use() const
{
	glUseProgram(program);
}

void GLSLProgram::UnUse() const
{
	glUseProgram(0);
}

GLint GLSLProgram::GetAttribLocation(const std::string& attribName) const
{
	return glGetAttribLocation(program, attribName.c_str());
}

GLuint GLSLProgram::GetProgramID() const
{
	return program;
}

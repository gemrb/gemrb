#include <cstdio>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "GLSLProgram.h"

using namespace GemRB;

std::string GLSLProgram::errMessage;

GLSLProgram* GLSLProgram::Create(std::string vertexSource, std::string fragmentSource)
{
	GLSLProgram* program = new GLSLProgram();
	if (!program->buildProgram(vertexSource, fragmentSource))
	{
		delete program;
		return NULL;
	}
	return program;
}

GLSLProgram* GLSLProgram::CreateFromFiles(std::string vertexSourceFileName, std::string fragmentSourceFileName)
{
	std::string vertexContent;
	std::string fragmentContent;
    
	std::ifstream fileStream(vertexSourceFileName.c_str());
	if(!fileStream.is_open()) 
	{
		GLSLProgram::errMessage = "GLSLProgram error: Can't open file: " + vertexSourceFileName;
		return NULL;
    }
	std::string line = "";
    while (!fileStream.eof()) 
	{
        std::getline(fileStream, line);
		line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
#ifdef USE_GL
		// remove precisions
		if (line.find("precision") == 0) continue;
#endif
        vertexContent.append(line + "\n");
    }
    fileStream.close();
	
	std::ifstream fileStream2(fragmentSourceFileName.c_str());
	if(!fileStream2.is_open()) 
	{
		GLSLProgram::errMessage = "GLSLProgram error: Can't open file: " + fragmentSourceFileName;
		return NULL;
    }
	while (!fileStream2.eof()) 
	{
        std::getline(fileStream2, line);
		line.erase(line.begin(), std::find_if(line.begin(), line.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
#ifdef USE_GL
		// remove precisions
		if (line.find("precision") == 0) continue;
#endif
        fragmentContent.append(line + "\n");
    }
	fileStream2.close();
	return GLSLProgram::Create(vertexContent, fragmentContent);
}

GLuint GLSLProgram::buildShader(GLenum type, std::string source)
{
    GLuint id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, 0);
    glCompileShader(id);
    GLint result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) 
	{
		char buff[512];
		glGetShaderInfoLog(id, sizeof(buff), 0, buff);
		GLSLProgram::errMessage = std::string(buff);
		glDeleteShader(id);
		return 0;
    }
    return id;
}

bool GLSLProgram::buildProgram(std::string vertexSource, std::string fragmentSource)
{
	program = 0;
    program = glCreateProgram();
	if (program == 0)
	{
		GLSLProgram::errMessage = "GLSLProgram error: glCreateProgram failed";
		glDeleteProgram(program);
		return false;
	}
	
    GLuint vertexId = buildShader(GL_VERTEX_SHADER, vertexSource);
	if (vertexId == 0)
	{
		glDeleteProgram(program);
		return false;
	}
    GLuint fragmentId = buildShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (fragmentId == 0) 
	{
		glDeleteProgram(program);
		return false;
	}

    glAttachShader(program, vertexId);
    glAttachShader(program, fragmentId);

    glLinkProgram(program);
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) 
	{
		char buff[512];
        glGetProgramInfoLog(program, sizeof(buff), 0, buff);
        GLSLProgram::errMessage = std::string(buff);
		glDeleteProgram(program);
		program = 0;
    }
    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);
	if (program != 0)
	{
		int count = -1;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count); 
		for(int i=0; i<count; i++)  
		{
			int name_len = -1, num = -1;
			GLenum type = GL_ZERO;
			char name[64];
			glGetActiveUniform(program, GLuint(i), sizeof(name) - 1, &name_len, &num, &type, name);
			name[name_len] = 0;
			storeUniformLocation(std::string(name));
		}
	}

    return program != 0;
}

GLint GLSLProgram::getUniformLocation(std::string uniformName)
{
	if (uniforms.find(uniformName) == uniforms.end())
	{
		GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform location";
		return -1;
	}
	return uniforms.at(uniformName);
}

void GLSLProgram::Release()
{
	if (program != 0) glDeleteProgram(program);
	delete this;
}

const std::string GLSLProgram::GetLastError()
{
	return GLSLProgram::errMessage;
}

bool GLSLProgram::storeUniformLocation(std::string uniformName)
{
	if (uniforms.find(uniformName) == uniforms.end())
	{
		GLint location = glGetUniformLocation(program, uniformName.c_str());
		if (location == -1)
		{
			GLSLProgram::errMessage = "GLSLProgram error: Invalid uniform location";
			return false;
		}
		uniforms[uniformName] = location;
		return true;
	}
	else
	{
		GLSLProgram::errMessage = "GLSLProgram error: Uniform already stored";
		return true;
	}
}

bool GLSLProgram::SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
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

bool GLSLProgram::SetUniformValue(std::string uniformName, const unsigned char size, GLsizei count, const GLint* value)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
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

bool GLSLProgram::SetUniformMatrixValue(std::string uniformName, const unsigned char size, GLsizei count, const GLfloat* value)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
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

bool GLSLProgram::SetUniformValue(std::string uniformName, const unsigned char size, GLfloat value1, GLfloat value2, GLfloat value3, GLfloat value4)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
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

bool GLSLProgram::SetUniformValue(std::string uniformName, const unsigned char size, GLint value1, GLint value2, GLint value3, GLint value4)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
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

void GLSLProgram::Use()
{
	glUseProgram(program);
}

void GLSLProgram::UnUse()
{
	glUseProgram(0);
}

GLint GLSLProgram::GetAttribLocation(std::string attribName)
{
	return glGetAttribLocation(program, attribName.c_str());
}

#include <cstdio>
#include <cstring>

#include "GLSLProgram.h"

using namespace GemRB;

char GLSLProgram::errMessage[512];

GLSLProgram* GLSLProgram::Create(const char* vertexSource, const char* fragmentSource)
{
	GLSLProgram* program = new GLSLProgram();
	if (!program->buildProgram(vertexSource, fragmentSource))
	{
		delete program;
		return NULL;
	}
	return program;
}


GLuint GLSLProgram::buildShader(GLenum type, const char* source)
{
    GLuint id = glCreateShader(type);
	glShaderSource(id, 1, &source, 0);
    glCompileShader(id);
    GLint result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) 
	{
		glGetShaderInfoLog(id, sizeof(GLSLProgram::errMessage), 0, GLSLProgram::errMessage);
        GLSLProgram::errMessage[strlen(GLSLProgram::errMessage)]='\0';
    }
    return id;
}

bool GLSLProgram::buildProgram(const char* vertexSource, const char* fragmentSource)
{
	program = 0;
    program = glCreateProgram();
	if (program == 0)
	{
		strcpy(GLSLProgram::errMessage, "GLSLProgram error: glCreateProgram failed\0");
		return false;
	}
	
    GLuint vertexId = buildShader(GL_VERTEX_SHADER, vertexSource);
	if (vertexId == 0) return false;
    GLuint fragmentId = buildShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (fragmentId == 0) return false;

    glAttachShader(program, vertexId);
    glAttachShader(program, fragmentId);

    glLinkProgram(program);
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) 
	{
        glGetProgramInfoLog(program, sizeof(GLSLProgram::errMessage), 0, GLSLProgram::errMessage);
        GLSLProgram::errMessage[strlen(GLSLProgram::errMessage)]='\0';
		glDeleteProgram(program);
		program = 0;
    }
    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);

    return program != 0;
}

GLint GLSLProgram::getUniformLocation(const char* uniformName)
{
	if (uniforms.find(uniformName) == uniforms.end())
	{
		strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform location\0");
		return -1;
	}
	return uniforms.at(uniformName);
}

void GLSLProgram::Release()
{
	if (program != 0) glDeleteProgram(program);
	delete this;
}

void GLSLProgram::GetLastError(char* msg , unsigned int msgSize)
{
	strncpy(msg, GLSLProgram::errMessage, msgSize);
}

bool GLSLProgram::StoreUniformLocation(const char* uniformName)
{
	if (uniforms.find(uniformName) == uniforms.end())
	{
		GLint location = glGetUniformLocation(program, uniformName);
		if (location == -1)
		{
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform location\0");
			return false;
		}
		uniforms[uniformName] = location;
		return true;
	}
	else
	{
		strcpy(GLSLProgram::errMessage, "GLSLProgram error: uniform already stored\0");
		return false;
	}
}

bool GLSLProgram::SetUniformValue(const char* uniformName, const unsigned char size, GLsizei count, const GLfloat* value)
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
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform size\0");
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const char* uniformName, const unsigned char size, GLsizei count, const GLint* value)
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
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform size\0");
			return false;
	}
}

bool GLSLProgram::SetUniformMatrixValue(const char* uniformName, const unsigned char size, GLsizei count, GLboolean transpose, const GLfloat* value)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	switch(size)
	{
		case 2:
			glUniformMatrix2fv(location, count, transpose, value);
			return true;
		case 3:
			glUniformMatrix3fv(location, count, transpose, value);
			return true;
		case 4:
			glUniformMatrix4fv(location, count, transpose, value);
			return true;
		default:
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform size\0");
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const char* uniformName, const unsigned char size, GLfloat value1, GLfloat value2, GLfloat value3, GLfloat value4)
{
	GLint location = getUniformLocation(uniformName);
	if (location == -1) return false;
	GLint loc;
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
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform size\0");
			return false;
	}
}

bool GLSLProgram::SetUniformValue(const char* uniformName, const unsigned char size, GLint value1, GLint value2, GLint value3, GLint value4)
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
			strcpy(GLSLProgram::errMessage, "GLSLProgram error: invalid uniform size\0");
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

GLint GLSLProgram::GetAttribLocation(const char* attribName)
{
	return glGetAttribLocation(program, attribName);
}

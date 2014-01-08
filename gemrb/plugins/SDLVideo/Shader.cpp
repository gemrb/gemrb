#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif
#include <cstdio>
#include <cstring>

#include "Shader.h"


ShaderOperationResult* Shader::BuildShader(GLenum type, const char* source)
{
	ShaderOperationResult* opResult = new ShaderOperationResult();
    GLuint id = glCreateShader(type);
	glShaderSource(id, 1, &source, 0);
    glCompileShader(id);
	opResult->Id = id;
    GLint result = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result != GL_TRUE) 
	{
        char tmp[2048];
        glGetShaderInfoLog(id, sizeof(tmp), 0, tmp);
		opResult->Id = 0;
        //sprintf(tmp, "%s shader compile error: %s", (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment", tmp);
		opResult->Message = tmp;
    }
    return opResult;
}

ShaderOperationResult* Shader::BuildProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint id = glCreateProgram();
	
    ShaderOperationResult* vertexShader = BuildShader(GL_VERTEX_SHADER, vertexSource);
	if (vertexShader->Id == 0) return vertexShader;
    ShaderOperationResult* fragmentShader = BuildShader(GL_FRAGMENT_SHADER, fragmentSource);
	if (fragmentShader->Id == 0) return fragmentShader;

    glAttachShader(id, vertexShader->Id);
    glAttachShader(id, fragmentShader->Id);

	ShaderOperationResult* opResult = new ShaderOperationResult();
	opResult->Id = id;

    glLinkProgram(id);
    GLint result = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &result);
    if (result != GL_TRUE) 
	{
        char tmp[2048];
		opResult->Id = 0;
        glGetProgramInfoLog(id, sizeof(tmp), 0, tmp);
        tmp[strlen(tmp)]='\0';
		opResult->Message = tmp;
    }

    glDeleteShader(vertexShader->Id);
    glDeleteShader(fragmentShader->Id);

    return opResult;
}

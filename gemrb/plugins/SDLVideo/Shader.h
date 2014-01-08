
class ShaderOperationResult
{
public:
	GLuint Id;
	char* Message; 
};

class Shader
{
public:
	static ShaderOperationResult* BuildShader(GLenum type, const char* source);
	static ShaderOperationResult* BuildProgram(const char* vertexSource, const char* fragmentSource);
};

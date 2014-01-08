
class ShaderOperationResult
{
public:
	unsigned Id;
	char* Message; 
};

class Shader
{
public:
	static ShaderOperationResult* BuildShader(GLenum type, const char* source);
	static ShaderOperationResult* Shader::BuildProgram(const char* vertexSource, const char* fragmentSource);
};

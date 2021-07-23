varying vec4 v_color;
varying vec2 v_texCoord;
varying vec2 v_stencilCoord;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	v_color = gl_Color;
	v_texCoord = vec2(gl_MultiTexCoord0);
	v_stencilCoord = v_texCoord; // vec2(gl_MultiTexCoord1);
}

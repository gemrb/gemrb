varying vec4 v_color;
varying vec2 v_texCoord;
varying vec2 v_stencilCoord;

uniform mat3 u_stencilMat;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	v_color = gl_Color;
	v_texCoord = vec2(gl_MultiTexCoord0);
	v_stencilCoord = (u_stencilMat * vec3(gl_Vertex.xy, 1.0)).xy;
}

attribute vec2 a_position;
attribute vec2 a_texCoord;
uniform mat4 u_matrix;
varying vec2 v_texCoord;
void main()
{
	gl_Position = u_matrix * vec4(a_position, 0.0, 1.0);
	v_texCoord = a_texCoord;
}
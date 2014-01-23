attribute vec2 a_position;
attribute vec2 a_texCoord;
attribute float a_alphaModifier;
attribute vec4 a_tint;
uniform mat4 u_matrix;
varying vec2 v_texCoord;
varying float v_alphaModifier;
varying vec4 v_tint;
void main()
{
	gl_Position = u_matrix * vec4(a_position, 0.0, 1.0);
	v_texCoord = a_texCoord;
	v_alphaModifier = a_alphaModifier;
	v_tint = a_tint;
}
attribute vec2 a_position;
uniform mat4 u_matrix;
void main()
{
	gl_Position = u_matrix * vec4(a_position, 0.0, 1.0);
}
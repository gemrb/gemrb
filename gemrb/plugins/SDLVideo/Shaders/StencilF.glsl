
varying vec2 v_texCoord;
uniform sampler2D s_texture;

uniform int u_channel;
void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
	if (u_channel == 1) {
		gl_FragColor = vec4(0, 0, 0, color.r);
	} else if (u_channel == 2) {
		gl_FragColor = vec4(0, 0, 0, color.g);
	} else if (u_channel == 3) {
		gl_FragColor = vec4(0, 0, 0, color.b);
	} else {
		gl_FragColor = vec4(0, 0, 0, color.a);
	}
}

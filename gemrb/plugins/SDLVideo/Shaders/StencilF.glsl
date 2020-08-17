
varying vec2 v_texCoord;
uniform sampler2D s_texture;

uniform int u_channel;
void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
	gl_FragColor = vec4(0, 0, 0, color[u_channel - 1]);
}

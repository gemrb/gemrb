precision highp float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;
uniform float u_alphaModifier;
uniform vec4 u_tint;
void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
	gl_FragColor = vec4(color.r*u_tint.r, color.g*u_tint.g, color.b*u_tint.b, color.a*u_alphaModifier);
}
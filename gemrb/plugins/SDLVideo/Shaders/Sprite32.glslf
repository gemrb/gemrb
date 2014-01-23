precision highp float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;
varying float v_alphaModifier;
varying vec4 v_tint;
void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
	gl_FragColor = vec4(color.r*v_tint.r, color.g*v_tint.g, color.b*v_tint.b, color.a * v_alphaModifier);
}
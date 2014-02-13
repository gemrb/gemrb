precision highp float;
uniform sampler2D s_texture;	// own texture
uniform sampler2D s_palette;	// palette 256 x 1 pixels
uniform sampler2D s_mask;		// optional mask
varying vec2 v_texCoord;
uniform float u_alphaModifier;
uniform vec4 u_tint;
void main()
{
	float alphaModifier = u_alphaModifier * texture2D(s_mask, v_texCoord).a;
	float index = texture2D(s_texture, v_texCoord).a;
	vec4 color = texture2D(s_palette, vec2((0.5 + index*255.0)/256.0, 0.5));
	gl_FragColor = vec4(color.r*u_tint.r, color.g*u_tint.g, color.b*u_tint.b, color.a * alphaModifier);
}
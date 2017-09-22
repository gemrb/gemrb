precision highp float;
uniform sampler2D s_texture;	// own texture
uniform sampler2D s_palette;	// palette 256 x 1 pixels
uniform sampler2D s_mask;		// optional mask
varying vec2 v_texCoord;
uniform float u_alphaModifier;
const vec3 lightColor = vec3(0.9, 0.9, 0.5);
const vec3 darkColor = vec3(0.2, 0.05, 0.0);
uniform int u_shadowMode;

void main()
{
	float alphaModifier = u_alphaModifier * texture2D(s_mask, v_texCoord).a;
	float index = texture2D(s_texture, v_texCoord).a;
	int iindex = int(index * 255.0);

	if ((0 == u_shadowMode) && (1 == iindex)) {
		gl_FragColor = vec4(255, 255, 255, 0);
	} else {
		vec4 color = texture2D(s_palette, vec2((0.5 + index*255.0)/256.0, 0.5));

		if (2 == u_shadowMode && (1 == iindex)) {
			color = vec4(color.r/2.0, color.g/2.0, color.b/2.0, 0.5);
		}

		float gray = (color.r + color.g + color.b)*0.333333;
		vec3 sepia = darkColor*(1.0 - gray) + lightColor*gray;
		gl_FragColor = vec4(sepia, color.a * alphaModifier);
	}
}

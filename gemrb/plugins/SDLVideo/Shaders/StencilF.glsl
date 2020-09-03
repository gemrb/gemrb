precision highp float;

varying vec2 v_texCoord;
uniform sampler2D s_texture;

uniform int u_channel;
uniform int u_dither;

void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
	gl_FragColor = vec4(0, 0, 0, color[u_channel - 1]);
	
	if (u_dither == 1 && gl_FragColor.a == 0.5) {
		// dithering is an alternative to BLIT_HALFTRANS
		vec2 p = vec2(floor(gl_FragCoord.x), floor(gl_FragCoord.y));
		if (mod(p.y, 2.0) == 0.0) {
			gl_FragColor.a = float(mod(p.x, 2.0) == 0.0);
		} else {
			gl_FragColor.a = float(mod(p.x, 2.0) != 0.0);
		}
	}
}

precision highp float;

//varying vec4 v_color; // glColor, we set this to the "tint" parameter
varying vec2 v_stencilCoord;

uniform sampler2D s_stencil;
uniform int u_channel;
uniform int u_dither;

void main()
{
	vec4 color = texture2D(s_stencil, v_stencilCoord);
	gl_FragColor = vec4(0, 0, 0, color[u_channel]);
	
	// FIXME: I feel like there is a better way of testing half alpha
	if (u_dither == 1 && gl_FragColor.a <= 0.51 && gl_FragColor.a >= 0.49) {
		// dithering is an alternative to BlitFlags::HALFTRANS
		vec2 p = vec2(floor(gl_FragCoord.x), floor(gl_FragCoord.y));
		if (mod(p.y, 2.0) == 0.0) {
			if (mod(p.x, 2.0) == 0.0) {
				gl_FragColor.a = 0.5;
			} else {
				gl_FragColor.a = 0.75;
			}
		} else if (mod(p.x, 2.0) != 0.0) {
			gl_FragColor.a = 0.5;
		} else {
			gl_FragColor.a = 0.75;
		}
	}
}

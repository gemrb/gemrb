precision highp float;

varying vec4 v_color;
varying vec2 v_texCoord;
varying vec2 v_stencilCoord;

uniform sampler2D s_sprite;
uniform sampler2D s_stencil;
uniform int u_greyMode;
uniform int u_channel;
uniform int u_stencil;
uniform int u_dither;
uniform int u_rgba;

void main() {
	vec4 color = texture2D(s_sprite, v_texCoord) * v_color;
	gl_FragColor = color;
	if (u_rgba == 0) {
		gl_FragColor.a = 1.0;
	}

	if (u_greyMode == 1) {
		float grey = (color.r + color.g + color.b)*0.333333;
		gl_FragColor = vec4(grey, grey, grey, gl_FragColor.a);
	} else if (u_greyMode == 2) {
		float grey = (color.r + color.g + color.b)*0.333333;
		gl_FragColor = vec4(grey + (21.0/256.0), grey, max(0.0, grey - (32.0/256.0)), gl_FragColor.a);
	}

	if (u_stencil == 1) {
		float stencilFactor = (1.0 - texture2D(s_stencil, v_stencilCoord)[u_channel]);

		if (u_dither == 1 && stencilFactor < 1.0) {
			float p = floor(gl_FragCoord.x) + floor(gl_FragCoord.y);
			gl_FragColor.a = mod(p, 2.0) * step(0.01, gl_FragColor.a) * gl_FragColor.a;
		} else {
			gl_FragColor.a *= stencilFactor;
		}
	}
}

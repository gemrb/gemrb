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
uniform float u_brightness;
uniform float u_contrast;

void main() {
	vec4 color = texture2D(s_sprite, v_texCoord) * v_color;

	// Additive Brightness
	color.rgb += (u_brightness-1.0);
	// Originals call the second parameter "Contrast" in UI and "Gamma Correction" in inis.
	// Looks like it's neither, just a multiplicator to brightness.
	color.rgb = color.rgb *= u_contrast;

	// Limit to 1.0
	color.rgb = clamp(color.rgb, 0.0, 1.0);

	// Actual gamma correction example.
//	color.rgb = pow(color.rgb, vec3(1.0 / u_contrast));
	// Actual contrast correction example.
//	color.rgb = ((color.rgb - 0.5) * max(u_contrast, 0.0)) + 0.5;

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

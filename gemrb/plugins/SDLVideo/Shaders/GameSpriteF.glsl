precision highp float;

varying vec2 v_texCoord;
varying vec4 v_color;
uniform sampler2D s_sprite;

uniform int u_greyMode;

void main()
{
	vec4 color = texture2D(s_sprite, v_texCoord) * v_color;

	if (u_greyMode == 1) {
		float grey = (color.r + color.g + color.b)*0.333333;
		gl_FragColor = vec4(grey, grey, grey, color.a);
	} else if (u_greyMode == 2) {
		float grey = (color.r + color.g + color.b)*0.333333;
		gl_FragColor = vec4(grey + (21.0/256.0), grey, max(0.0, grey - (32.0/256.0)), color.a);
	} else {
		gl_FragColor = color;
	}
}


varying vec2 v_texCoord;
varying vec4 v_color;
uniform sampler2D s_texture;

uniform int u_greyMode;
uniform int u_shadowMode;
uniform vec3 u_ck;

void main()
{
	vec4 color = texture2D(s_texture, v_texCoord);
/*
	if (u_shadowMode == 1) {
		if (color.rgb == u_ck) {
			color.a = 0.0;
		}
	} else if (u_shadowMode == 2) {
		if (color.rgb == u_ck) {
			color.a = 0.5;
		}
	}
*/
	if (u_greyMode == 1) {
		float grey = (color.r + color.g + color.b)*0.333333;
		color = vec4(grey, grey, grey, color.a);
	} else if (u_greyMode == 2) {
		float grey = (color.r + color.g + color.b)*0.333333;
		color = vec4(grey + (21.0/256.0), grey, max(0.0, grey - (32.0/256.0)), color.a);
	}

	gl_FragColor = vec4(color.rgb * v_color.rgb, color.a);
}

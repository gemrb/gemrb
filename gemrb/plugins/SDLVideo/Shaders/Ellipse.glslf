precision highp float;
uniform float u_radiusX;
uniform float u_radiusY;
uniform float u_thickness;
uniform float u_support;
uniform vec4 u_color;
varying vec2 v_texCoord;
void main()
{
	float x = v_texCoord.x;
	float y = v_texCoord.y;

	float x_mid = u_radiusX + u_thickness/2.0;
	float y_mid = u_radiusY + u_thickness/2.0;
	float distance1 = sqrt(x*x/(x_mid*x_mid) + y*y/(y_mid*y_mid));
	float width1 = fwidth(distance1) * u_support/1.25;
	float alpha1  = smoothstep(1.0 - width1, 1.0 + width1, distance1);

	x_mid = u_radiusX - u_thickness/2.0;
	y_mid = u_radiusY - u_thickness/2.0;
	float distance2 = sqrt(x*x/(x_mid*x_mid) + y*y/(y_mid*y_mid));
	float width2 = fwidth(distance2) * u_support/1.25;
	float alpha2  = smoothstep(1.0 + width2, 1.0 - width2, distance2);

	gl_FragColor = vec4(u_color.rgb, (1.0 - max(alpha1, alpha2))*u_color.a);
}
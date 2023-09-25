#version 330

in vec3 fragNormal;
in vec2 fragTexCoord;

uniform sampler2D texture0;

void main()
{
	vec3 l = normalize(vec3(0.0, 1.0, 2.0));
	float diffuse = max(dot(fragNormal, l), 0.0);
	float ambient = 0.3;
	gl_FragColor = vec4(0.0, (ambient + diffuse) * 1.0, 0.0, 1.0);
	//gl_FragColor = vec4(fragNormal.x, fragNormal.y, fragNormal.z, 1.0);
};
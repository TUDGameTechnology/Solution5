#version 450

uniform sampler2D tex;
in vec2 texCoord;
in vec3 normal;
in vec3 lightDirection;
in vec3 eyeCoord; 

out vec4 frag;

void main() {
	const float amb = 0.2;
	const float diff = 1.0;
	const float spec = 2.0;
	const float n = 16.0;

	vec4 ambient = vec4(amb, amb, amb, 1.0);

	vec3 nor = normalize(normal);
	vec3 lightDir = normalize(lightDirection);
	vec4 diffuse = max(dot(lightDir, nor), 0.0) * vec4(diff, diff, diff, 1.0);
	
	vec3 halfVector = normalize(lightDir - normalize(eyeCoord));
	vec4 specular = pow(max(0.0, dot(halfVector, nor)), n) * vec4(spec, spec, spec, 1.0);

	vec4 light = ambient + diffuse + specular;

	frag = light * texture(tex, texCoord);
}

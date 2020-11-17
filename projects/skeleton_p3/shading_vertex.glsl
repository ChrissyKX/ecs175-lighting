#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;
layout(location = 2) in vec3 vertexNormal_modelspace;

// Output data ; will be interpolated for each fragment.
//out vec3 normal_modelspace;
out vec3 fragmentColor;

// Values that stay constant for the whole mesh.
uniform mat4 MVP;
uniform vec3 ka;  // ka, kd, ks
uniform vec3 kd;
uniform vec3 ks;
uniform vec2 i;  // Ia, Il
uniform vec3 x;  // position of the light source
uniform vec3 f;  // from point
uniform int n;  // Phong constant

uniform bool divider;

void main(){

	if (!divider) {
		gl_Position =  MVP * vec4(vertexPosition_modelspace,1);

		//	normal_modelspace = normalize(vertexNormal_modelspace);  // unit normal vector
		vec3 l = normalize(x - vertexPosition_modelspace);  // unit light vector
		vec3 r = -1 * l + 2 * dot(l, vertexNormal_modelspace) * vertexNormal_modelspace;  // unit reflection vector
		vec3 v = normalize(f - vertexPosition_modelspace);  // unit view vector
		float K = length(x - vertexPosition_modelspace);  // distance between point and light source
		fragmentColor = ka * i.x + i.y * (kd * dot(l, vertexNormal_modelspace) + ks * pow (dot(r, v), n)) / (length(f - vertexPosition_modelspace) + K);
	} else {
		gl_Position = vec4(vertexPosition_modelspace, 1);
		fragmentColor = vec3(1, 1, 1);
	}

}

#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUv;

out vec2 uv;
out vec3 worldPosition;

uniform mat4 world, view, projection;

uniform sampler2D terrainTex;

void main()
{
	vec3 pos = aPos;
	//object space offset

	vec4 worldPos = world * vec4(pos, 1.0);
	//world space offsets
	//worldPos.y += texture(terrainTex, vUv).r * 100f;

	gl_Position = projection * view * world * worldPos;
	uv = vUv;

	worldPosition = mat3(world) * aPos;
}
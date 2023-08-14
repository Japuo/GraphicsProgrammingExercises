#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D spikeTex;

void main()
{
    vec4 texColor = texture(spikeTex, texCoord);

    FragColor = texColor;
}
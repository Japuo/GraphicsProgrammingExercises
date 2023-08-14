#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 normal;
} gs_in[];

out vec2 texCoord;

const float MAGNITUDE = 0.1;

uniform mat4 projection;

void GenerateOutwardTriangles(int index)
{
    vec4 centerPosition = gl_in[index].gl_Position;
    
    vec4 offset = vec4(gs_in[index].normal * MAGNITUDE, 0.0);
    
    gl_Position = projection * (centerPosition + offset);
    texCoord = vec2(0.3, 0.1); // Top vertex
    EmitVertex();
    
    gl_Position = projection * (centerPosition + vec4(-offset.y, offset.x, 0.0, 0.0));
    texCoord = vec2(0.0, 0.8); // Left vertex
    EmitVertex();
    
    gl_Position = projection * (centerPosition - offset);
    texCoord = vec2(0.5, 0.9); // Bottom vertex
    EmitVertex();
    
    EndPrimitive();
}

void main()
{
    GenerateOutwardTriangles(0);
    GenerateOutwardTriangles(1);
    GenerateOutwardTriangles(2);
}

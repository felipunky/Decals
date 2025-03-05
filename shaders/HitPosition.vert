#version 150 core
precision mediump float;

//layout (location = 0)
in vec3 VertexPosition;
//layout (location = 1)
//in vec2 VertexTextureCoords;

//out vec2 texCoords;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    //texCoords = VertexTextureCoords;
    vec4 worldPos = projection * view * model * vec4(VertexPosition, 1.0);
    gl_Position = worldPos;
}

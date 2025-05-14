precision mediump float;

//in vec2 texCoords;

out vec4 FragColor;

uniform mat4 model;                                        
uniform mat4 view;                                         
uniform mat4 projection; 

uniform vec3 color;

void main()
{    
    FragColor = vec4(color, 1.);
}

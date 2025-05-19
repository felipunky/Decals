precision highp float;

//layout (location = 0)
in vec3 VertexPosition;
//layout (location = 1)
in vec2 VertexTextureCoords;

out vec2 TexCoords;  
 
uniform mat4 decalProjector;   

void main()                                                
{              
    TexCoords = VertexTextureCoords;
    gl_Position = vec4(VertexPosition, 1.0);              
}                                                      

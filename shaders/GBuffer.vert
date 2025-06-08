precision mediump float;

//layout (location = 0)
in vec3 VertexPosition;
//layout (location = 1)
in vec3 VertexNormals;
//layout (location = 2)
in vec2 VertexTextureCoords;
//layout (location = 3)
in vec4 VertexTangents;

out vec3 positions;                             
out vec2 texCoords;     
out mat3 TBN;                               

uniform mat4 model;                                        
uniform mat4 view;                                         
uniform mat4 projection;  
uniform mat4 decalProjector;    
uniform int iFlipper;      

void main()                                                
{                

  texCoords = VertexTextureCoords;
  vec3 vertexNormals = VertexNormals; 
  if (iFlipper == 1)
  {
    texCoords.y = 1. - texCoords.y;
  }
          
  vec4 worldPos = model * vec4(VertexPosition.xyz, 1.);    
  vec4 objPos = projection * view * worldPos;    
  positions = objPos.xyz;
  gl_Position = decalProjector * vec4(VertexPosition, 1.0);              
}                                                      

precision mediump float;

in vec3 VertexPosition;
in vec3 VertexNormals;
in vec2 VertexTextureCoords;
in vec4 VertexTangents;

out vec3 Positions;                             
out vec2 TexCoords;
out mat3 TBN;   
out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 model;                                        
uniform mat4 view;                                         
uniform mat4 projection;   
uniform vec3 viewPos;
uniform float iTime;

mat2 rot(const in float a)
{
  vec2 sinCos = vec2(sin(a), cos(a));
  return mat2(sinCos.y, -sinCos.x,
              sinCos.x,  sinCos.y);
}

void main()                                                
{               

  TexCoords = VertexTextureCoords;

  vec4 worldPos = model * vec4(VertexPosition.xyz, 1.); 
  Positions = worldPos.xyz;
  
  mat3 normalMatrix = transpose(inverse(mat3(model)));
  vec3 T = normalize(normalMatrix * VertexTangents.xyz);
  vec3 N = normalize(normalMatrix * VertexNormals.xyz);
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);

  TBN = transpose(mat3(T, B, N));

  float speed = 0.5 * iTime;
  vec2 sinCosNormalized = vec2(sin(speed), cos(speed))*0.5+0.5;

  vec3 lightPos = vec3(sinCosNormalized.x, 1.0, sinCosNormalized.y);

  TangentLightPos = TBN * lightPos;
  TangentViewPos  = TBN * viewPos;
  TangentFragPos  = TBN * Positions;
        
  vec4 objPos = projection * view * worldPos;
  
  gl_Position = objPos;              
}                                                      

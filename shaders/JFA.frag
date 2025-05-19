precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform sampler2D iChannel0;
uniform int iFrame;
//uniform vec2 iResolution;

void main()
{ 
    vec2 uv = TexCoords;

    vec4 backBuffer = texture( iChannel0, uv );
    
    FragColor = vec4( ( backBuffer.rgb + 0.001 ), 1);
}
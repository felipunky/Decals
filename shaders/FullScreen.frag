precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform vec2 iResolution;
uniform sampler2D iChannel0;

void main()
{ 
    vec2 uv = TexCoords;//(-iResolution + 2.0 * gl_FragCoord.xy) / iResolution.y;
    FragColor = texture( iChannel0, uv );
}
precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform vec2 iResolution;
uniform sampler2D iChannel0;

//uniform float iDistanceWidth;

//const float c_distanceWidth = 16.0;

void main()
{
    vec2 uv = gl_FragCoord.xy * 2.0 / iResolution;
    FragColor = texture( iChannel0, uv );
}

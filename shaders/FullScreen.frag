precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform vec2 iResolution;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform float iSmoothness;
//uniform float iDistanceWidth;

//const float c_distanceWidth = 16.0;

void main()
{
    vec2 uv = TexCoords;
    float sdf = texture( iChannel0, uv ).r;
    vec3 col = mix( vec3( 0 ), texture( iChannel1, uv ).rgb, smoothstep( 0., iSmoothness, sdf ) );
    FragColor //= vec4( (sdf>0.0) ? vec3(0.9,0.6,0.3) : vec3(0.65,0.85,1.0), 1. );
                = vec4( col, 1.0 );
}

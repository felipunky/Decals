precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform vec2 iResolution;
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;
uniform sampler2D iChannel4;
uniform int iScale;
uniform bool iAlpha;
uniform float iSmoothness;
uniform int iTexture;

//uniform float iDistanceWidth;

//const float c_distanceWidth = 16.0;

void main()
{
    vec2 uv = gl_FragCoord.xy * float( iScale ) / iResolution;
    if (iTexture == 1)
    {
        FragColor = texture( iChannel0, uv );
    }
    else if (iTexture == 2)
    {
        FragColor = texture( iChannel1, uv );
    }
    else if (iTexture == 3)
    {
        FragColor = texture( iChannel2, uv );
    }
    else if (iTexture == 4)
    {
        FragColor = texture( iChannel3, uv );
    }
    else if (iAlpha)
    {
        float sdf = texture( iChannel4, uv ).r;
        FragColor = vec4( mix( vec3( 0 ), vec3( 1 ), smoothstep( 0., iSmoothness, sdf ) ), 1 );
    }
    if ( uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 )
    {
        FragColor *= 0.;
    }
}

precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform int iFrame;
uniform vec2 iResolution;
uniform float iAlphaCut;
uniform float iMaxSteps;

vec4 StepJFA (in vec2 fragCoord, in float level)
{
    level = clamp(level, 0.0, iMaxSteps);
    vec2 stepwidth = vec2( floor( exp2( iMaxSteps - level ) + 0.5 ) );
    
    float bestDistance = 9999.0;
    vec2 bestCoord = vec2(0.0);
    
    float bestDistance0 = 9999.0;
    vec2 bestCoord0 = vec2(0.0);
    
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 sampleCoord = ( fragCoord + ( vec2( x, y ) * stepwidth ) ) / iResolution;
            
            vec4 data = texture( iChannel0, sampleCoord );
            
            // Inside.
            vec2 seedCoord = data.xy;
            float dist = length(seedCoord - fragCoord);
            if ((seedCoord.x != 0.0 || seedCoord.y != 0.0) && dist < bestDistance)
            {
                bestDistance = dist;
                bestCoord = seedCoord;
            }
            
            // Outside.
            vec2 seedCoord0 = data.zw;
            float dist0 = length(seedCoord0 - fragCoord);
            if ((seedCoord0.x != 0.0 || seedCoord0.y != 0.0) && dist0 < bestDistance0)
            {
                bestDistance0 = dist0;
                bestCoord0 = seedCoord0;
            }
        }
    }
    
    return vec4(bestCoord, bestCoord0);
}

void main()
{ 
    vec2 uv = TexCoords;
    vec2 fragCoordUnnormalized = ( ( uv * iResolution ) + .5 );
    
    if ( iFrame == 0 )
    {
        vec4 initialTexture = texture( iChannel1, uv );
        bool mask = initialTexture.a > iAlphaCut;
        FragColor = ( mask ? vec4( fragCoordUnnormalized, 0.0, 0.0 ) : vec4( 0.0 ) );
        FragColor = ( !mask ? vec4( 0.0, 0.0, fragCoordUnnormalized ) : vec4( 0.0 ) );
    }
    else if ( iFrame <= int( iMaxSteps + 1. ) )
    {
        vec4 bestCoords = StepJFA( fragCoordUnnormalized, floor( float( iFrame - 1 ) ) );
        FragColor = bestCoords;
    }
    else
    {
        FragColor = texture( iChannel0, uv );
    }
}

precision highp float;
                                                         
in vec2 TexCoords;

//layout (location = 0)
out vec4 FragColor;

uniform sampler2D iChannel0;
uniform vec2 iResolution;
//uniform sampler2D iChannel1;
//uniform float iSmoothness;
uniform float iDistanceWidth;

//const float c_distanceWidth = 16.0;

void main()
{
    vec2 adjustedFragCoord = ( TexCoords * iResolution ) + .5;
    vec2 uv = adjustedFragCoord / iResolution;
    
    vec4 jfa = texture( iChannel0, uv );
    
    vec2 insideCoords = jfa.xy;
    vec2 outsideCoords = jfa.zw;
    
    // calculate distances
    float outsideDist = clamp(length(outsideCoords-adjustedFragCoord) / iDistanceWidth, 0.0, 1.0);
    float insideDist  = clamp(length(insideCoords-adjustedFragCoord)  / iDistanceWidth, 0.0, 1.0);
    
    // calculate output distance
    float signedDistance = 0.5 + outsideDist * 0.5 - insideDist * 0.5;

    FragColor = vec4( signedDistance );
}

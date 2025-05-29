precision highp float;

in vec2 TexCoords;
in vec3 WorldPos;

out vec4 FragColor;

uniform mat4 model;                                        
uniform mat4 view;                                         
uniform mat4 projection; 
uniform mat4 decalProjector;

uniform vec2 iResolution;
uniform int iScale;
uniform float iFlip;
uniform float iBlend;
uniform float iSmoothness;
uniform bool iAlpha;
uniform bool iShowSDFBox;

uniform sampler2D iChannel1;
uniform sampler2D iChannel0;
uniform sampler2D iDepth;
uniform sampler2D iSDF;

uniform float bias;

//#define BIAS 0.0001

float SDFBox(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

bool CheckBox(const in vec3 uv, const in float bias)
{
    float oneMinusBias = 1.0 - bias;
    return (uv.x >= oneMinusBias || uv.x <= bias || 
            uv.y >= oneMinusBias || uv.y <= bias || 
            uv.z >= oneMinusBias || uv.z <= bias);
}

void main()
{
    vec3 decalUV = WorldPos;
    vec2 texCoords = TexCoords;

    vec3 boxSize = vec3(0.5);
    float depth = texture(iDepth, decalUV.xy).r;
    if (abs(decalUV.z - bias) > (depth))
    {
        boxSize.z = 0.;
        //decalUV += 10000.;
    }
    
    float boxSDF = SDFBox(decalUV-.5, boxSize);
    float boxSDFRemapped = boxSDF * 0.5 + 0.5;

    if (iFlip == 1.0)
    {
        decalUV.y = 1. - decalUV.y;
    }

    decalUV.xy = fract(decalUV.xy * float(iScale));

    vec4 projectedDecal = texture(iChannel0, decalUV.xy);
    vec4 albedoMap      = texture(iChannel1, texCoords);
    
    if (iAlpha)
    {
        float sdf       = texture(iSDF,      WorldPos.xy).r;
        projectedDecal.rgb = mix( albedoMap.rgb, projectedDecal.rgb, smoothstep( 0., iSmoothness, sdf ) );
    }
        
    float minDecalsUV = (max(decalUV.x, decalUV.y), decalUV.z);

    float showSDFBox = (iShowSDFBox ? 0.0 : 1.0);

    vec3 colorOut = mix( projectedDecal.rgb * showSDFBox, albedoMap.rgb, smoothstep( 0.0, minDecalsUV * iBlend, boxSDF ) );
    FragColor = vec4( colorOut, 1.0 );
}

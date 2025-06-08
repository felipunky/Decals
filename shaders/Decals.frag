precision highp float;

in vec3 WorldPos;
in vec2 TexCoords;

// Uncomment the following for Mac
#define __EMSCRIPTEN__

#ifdef __EMSCRIPTEN__
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 Normal;
#else
out vec4 FragColor;
out vec4 Normal;
#endif

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

// Decal Albedo
uniform sampler2D iChannel0;
// Albedo
uniform sampler2D iChannel1;
// Decal Normal
uniform sampler2D iChannel2;
// Normal
uniform sampler2D iChannel3;

uniform sampler2D iDepth;
uniform sampler2D iSDF;

uniform float bias;

//#define BIAS 0.0001

float SDFBox(vec3 p, vec3 b)
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// https://blog.selfshadow.com/publications/blending-in-detail/
vec3 RNM( vec3 normalMap, vec3 decalsNormals )
{
    normalMap *= 2.0 - 1.0;
    decalsNormals *= 2.0 - 1.0;
    vec3 n1 = normalMap     * vec3( 2,  2, 2) + vec3(-1, -1,  0);
    vec3 n2 = decalsNormals * vec3(-2, -2, 2) + vec3( 1,  1, -1);
    vec3 n  = n1*dot(n1, n2)/n1.z - n2;
    return n;
}

void main()
{
    vec3 decalUV = WorldPos;
    vec2 texCoords = TexCoords;

    vec2 reciprocalDecalResolution = 1. / vec2( textureSize( iChannel0, 0 ) ); 
    float maxReciprocalDecalResolution = max( reciprocalDecalResolution.x, reciprocalDecalResolution.y );

    vec3 boxSize = vec3(0.5) + iBlend * maxReciprocalDecalResolution;
    float depth = texture(iDepth, decalUV.xy).r;
    if (abs(decalUV.z - bias) > (depth))
    {
        boxSize.z = 0.;
    }
    
    float boxSDF = SDFBox( decalUV - 0.5, boxSize );

    if (iFlip == 1.0)
    {
        decalUV.y = 1. - decalUV.y;
    }

    decalUV.xy = fract( decalUV.xy * float( iScale ) );

    vec4 projectedDecal = texture( iChannel0, decalUV.xy );
    vec4 decalsNormals  = texture( iChannel2, decalUV.xy );

    vec4 albedoMap      = texture( iChannel1, texCoords );
    vec4 normalMap      = texture( iChannel3, texCoords );

    if (iAlpha)
    {
        float sdf          = texture(iSDF,      WorldPos.xy).r;
        float renderedSDF  = smoothstep( 0.0, iSmoothness, sdf );
        projectedDecal.rgb = mix( albedoMap.rgb, projectedDecal.rgb, renderedSDF );
        decalsNormals.xyz   = mix( normalMap.xyz, decalsNormals.xyz,  renderedSDF );
    }

    float smoothnessBox = maxReciprocalDecalResolution * iBlend * 10.0;
    float showSDFBox = (iShowSDFBox ? 0.0 : 1.0);

    //vec2 fWidthDecalUV = fwidth( decalUV.xy );
    //float pDer = ( fWidthDecalUV.x + fWidthDecalUV.y );

    //vec3 debug = ( dFdx( decalUV.y ) < 0.1 ? vec3( 1.0, 0.0, 0.0 ) : projectedDecal.rgb );

    float renderBoxSDF = smoothstep( -smoothnessBox, smoothnessBox, boxSDF );

    vec3 colorOut = mix( projectedDecal.rgb * showSDFBox, albedoMap.rgb, renderBoxSDF );
    FragColor = vec4( colorOut, 1.0 );

    vec3 normals = mix( decalsNormals.xyz, normalMap.xyz, renderBoxSDF );
    normals = RNM( normalMap.xyz, normals ) * 0.5 + 0.5;
    Normal = vec4( mix( normals, normalMap.xyz, 1.-showSDFBox ), 1.0 );
}

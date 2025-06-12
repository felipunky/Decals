// Heavily inspired by https://learnopengl.com/code_viewer_gh.php?code=src/6.pbr/1.2.lighting_textured/1.2.pbr.fs

precision mediump float;

in vec3 Positions;                               
in vec2 TexCoords;
in mat3 TBN; 
in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D gAlbedo;
uniform sampler2D gNormal;
uniform sampler2D gMetallic;
uniform sampler2D gRoughness;
//uniform sampler2D gAO;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform float iTime;
uniform float iFlipAlbedo;
uniform int iNormals;
uniform bool iPBR;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
/*vec3 getNormalFromMap(const in vec2 texCoords)
{
    vec3 tangentNormal = texture(gNormal, texCoords).xyz * 2.0 - 1.0;
    return normalize(tangentNormal);
}*/
vec3 getNormalFromMap(const in vec2 texCoords)
{
    vec3 tangentNormal = texture(gNormal, texCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(Positions);
    vec3 Q2  = dFdy(Positions);
    vec2 st1 = dFdx(texCoords);
    vec2 st2 = dFdy(texCoords);

    vec3 N   = normalize(Normal);
    vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B   = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{             
    vec3 FragPos    = Positions;//texture(gPosition, TexCoords).xyz;
    vec2 texCoordsAlbedo = TexCoords;
    if (iFlipAlbedo == 1.0)
    {
        texCoordsAlbedo.y = 1. - texCoordsAlbedo.y;
    }
    vec3 albedo = texture(gAlbedo, texCoordsAlbedo).rgb;
    albedo = pow(albedo, vec3(2.2));

    if (iNormals == 1)
    {
        vec3 lightP = lightPos - FragPos;
                    //= TangentLightPos - TangentFragPos;
        vec3 L = normalize(lightP);
        vec3 V = normalize(viewPos - FragPos);
                           //TangentViewPos - TangentFragPos);

        vec3 N = getNormalFromMap(texCoordsAlbedo);
        vec3 halfWayVector = normalize(L + V);
        if (iPBR)
        {
            //vec3 H = halfWayVector;
            float metallic  = texture(gMetallic,  texCoordsAlbedo).r;
            float roughness = texture(gRoughness, texCoordsAlbedo).r;

            vec3 F0 = vec3(0.04); 
            F0 = mix(F0, albedo, metallic);

            vec3 Lo = vec3(0.0);
            float dist = length(lightP);
            float attenuation = 1.0 / (dist * dist);

            vec3 lightColour = vec3(1.0)*150.;
            vec3 radiance = lightColour * attenuation;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, halfWayVector, roughness);   
            float G   = GeometrySmith(N, V, L, roughness);      
            vec3 F    = fresnelSchlick(max(dot(halfWayVector, V), 0.0), F0);
           
            vec3 numerator    = NDF * G * F; 
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
            vec3 specular = numerator / denominator;
        
            // kS is equal to Fresnel
            vec3 kS = F;
            // for energy conservation, the diffuse and specular light can't
            // be above 1.0 (unless the surface emits light); to preserve this
            // relationship the diffuse component (kD) should equal 1.0 - kS.
            vec3 kD = vec3(1.0) - kS;
            // multiply kD by the inverse metalness such that only non-metals 
            // have diffuse lighting, or a linear blend if partly metal (pure metals
            // have no diffuse light).
            kD *= 1.0 - metallic;	  

            // scale light by NdotL
            float NdotL = max(dot(N, L), 0.0);        

            // add to outgoing radiance Lo
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again

            // ambient lighting (note that the next IBL tutorial will replace 
            // this ambient lighting with environment lighting).
            vec3 ambient = vec3(0.03) * albedo;// * ao;
    
            vec3 color = ambient + Lo;

            // HDR tonemapping
            color = color / (color + vec3(1.0));
            // gamma correct
            color = pow(color, vec3(1.0/2.2)); 

            FragColor = vec4(color, 1.0);
        }
        else
        {
            vec3 ambient = 0.1 * albedo;

            float dif = max(0.0, dot(L, N));
            //float spe = pow(max(dot(N, halfWayVector), 0.0), 32.0);
            float spe = pow(max(dot(reflect(-L, N), V), 0.), 32.);
            
            vec3 diffuse = albedo * dif;
            vec3 specular = vec3(0.2) * spe;

            vec3 color = ambient + diffuse + specular;
            //color = color / (color + vec3(1.0));

            FragColor = vec4(pow(color, vec3(0.45454)), 1.0);
        }
    }

    else
    {
        FragColor //= vec4(albedo, 1.0);
                  = vec4(pow(albedo, vec3(0.45454)), 1.0);
    }

}

/*============================================================

This shader combines the outside distance from buf A with the
inside distance from buf B to make a true SDF texture.

============================================================*/

// Shared constant between Buf C and Image
// The sdf texture is 1/c_sdfShrinkFactor in size
const float c_sdfShrinkFactor = 4.0; 

const float c_distanceWidth = 8.0; // This is the width in pixels that it transitions from outside to inside

//============================================================
void DecodeData (in vec4 data, out vec2 coord, out vec3 color)
{
    coord = data.xy;
    color.x = floor(data.z / 256.0) / 255.0;
    color.y = mod(data.z, 256.0) / 255.0;
    color.z = mod(data.w, 256.0) / 255.0;
}

//============================================================
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 adjustedFragCoord = floor(fragCoord) * c_sdfShrinkFactor - 0.5;
    
    // get our data
    vec2 uv = adjustedFragCoord / iResolution.xy;    
    vec4 outsideData = texture(iChannel0, uv);
    vec4 insideData = texture(iChannel1, uv);
    
    // decode outside data
    vec2 outsideCoord;
    vec3 outsideColor;
    DecodeData(outsideData, outsideCoord, outsideColor);

    // decode inside data
    vec2 insideCoord;
    vec3 insideColor;
    DecodeData(insideData, insideCoord, insideColor);   
    
    // calculate distances
    float outsideDist = clamp(length(outsideCoord-adjustedFragCoord) / c_distanceWidth, 0.0, 1.0);
    float insideDist  = clamp(length(insideCoord-adjustedFragCoord)  / c_distanceWidth, 0.0, 1.0);
    
    // calculate output distance
    float signedDistance = 0.5 + outsideDist * 0.5 - insideDist * 0.5;
        
    // set the color based on that distance
    fragColor = vec4(signedDistance);
}
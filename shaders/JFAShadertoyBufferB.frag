#define DIST_OUTSIDE 0

/*============================================================

This shader is the same in Buf A and Buf B with different
values for DIST_OUTSIDE.  Buf A finds the distance outside
of the shapes, while Buf B finds the distance inside of
the shapes.

Buf C combines the information to make a true SDF texture
which is used to do rendering in the Image tab.

============================================================*/

// how many JFA steps to do.  2^c_maxSteps is max image size on x and y
const float c_maxSteps = 10.0;

// circle info
const int c_numCircles = 10;
const vec2 c_circleRadius = vec2(0.01, 0.05);  // min, max

// line info
const int c_numLines = 10;
const vec4 c_lineSize = vec4(0.01, 0.01, 0.3, 0.1); // vec2 min, max 

// squiggle info
const int c_numSquiggles = 10;
const vec4 c_squiggleSize = vec4(0.01, 0.01, 0.3, 0.1); // vec2 min, max 

//============================================================
float UDFatLineSegment (in vec2 coords, in vec2 A, in vec2 B, in float height)
{    
    // calculate x and y axis of box
    vec2 xAxis = normalize(B-A);
    vec2 yAxis = vec2(xAxis.y, -xAxis.x);
    float width = length(B-A);
    
	// make coords relative to A
    coords -= A;
    
    vec2 relCoords;
    relCoords.x = dot(coords, xAxis);
    relCoords.y = dot(coords, yAxis);
    
    // calculate closest point
    vec2 closestPoint;
    closestPoint.x = clamp(relCoords.x, 0.0, width);
    closestPoint.y = clamp(relCoords.y, -height * 0.5, height * 0.5);
    
    return length(relCoords - closestPoint);
}

//============================================================
// Hash without sine from https://www.shadertoy.com/view/4djSRW
#define HASHSCALE1 .1031
#define HASHSCALE3 vec3(.1031, .1030, .0973)
float hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  1 out, 3 in...
float hash13(vec3 p3)
{
	p3  = fract(p3 * HASHSCALE1);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//----------------------------------------------------------------------------------------
//  2 out, 1 in...
vec2 hash21(float p)
{
	vec3 p3 = fract(vec3(p) * HASHSCALE3);
	p3 += dot(p3, p3.yzx + 19.19);
	return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

//----------------------------------------------------------------------------------------
///  2 out, 2 in...
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

//----------------------------------------------------------------------------------------
///  2 out, 3 in...
vec2 hash23(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract(vec2((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y));
}

//----------------------------------------------------------------------------------------
///  3 out, 2 in...
vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

//----------------------------------------------------------------------------------------
///  3 out, 3 in...
vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * HASHSCALE3);
    p3 += dot(p3, p3.yxz+19.19);
    return fract(vec3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

//============================================================
vec4 EncodeData (in vec2 coord, in vec3 color)
{
    vec4 ret = vec4(0.0);
    ret.xy = coord;
    ret.z = floor(color.x * 255.0) * 256.0 + floor(color.y * 255.0);
    ret.w = floor(color.z * 255.0);
    return ret;
}

//============================================================
void DecodeData (in vec4 data, out vec2 coord, out vec3 color)
{
    coord = data.xy;
    color.x = floor(data.z / 256.0) / 255.0;
    color.y = mod(data.z, 256.0) / 255.0;
    color.z = mod(data.w, 256.0) / 255.0;
}

//============================================================
vec4 StepJFA (in vec2 fragCoord, in float level)
{
    level = clamp(level, 0.0, c_maxSteps);
    float stepwidth = floor(exp2(c_maxSteps - level)+0.5);
    
    float bestDistance = 9999.0;
    vec2 bestCoord = vec2(0.0);
    vec3 bestColor = vec3(0.0);
    
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 sampleCoord = fragCoord + vec2(x,y) * stepwidth;
            
            vec4 data = texture( iChannel0, sampleCoord / iChannelResolution[0].xy);
            vec2 seedCoord;
            vec3 seedColor;
            DecodeData(data, seedCoord, seedColor);
            float dist = length(seedCoord - fragCoord);
            if ((seedCoord.x != 0.0 || seedCoord.y != 0.0) && dist < bestDistance)
            {
                bestDistance = dist;
                bestCoord = seedCoord;
                bestColor = seedColor;
            }
        }
    }
    
    return EncodeData(bestCoord, bestColor);
}

//============================================================
vec4 DrawShapes (in vec2 fragCoord)
{
    #if DIST_OUTSIDE == 1
    	vec4 ret = vec4(0.0);
    #else
    	vec4 ret = EncodeData(fragCoord, vec3(0.0, 1.0, 0.0));
    #endif
    
    float aspectRatio = iResolution.x / iResolution.y;
    vec2 uv = (fragCoord / iResolution.xy);
    uv.x *= aspectRatio;    
    
    // draw circles
    for (int i = 0; i < c_numCircles; ++i)
    {
        vec2 posuv = hash22(vec2(i, iDate.w));
        posuv.x *= aspectRatio;
        
        vec3 color = hash32(vec2(i, iDate.w));
        float radius = mix(c_circleRadius.x, c_circleRadius.y, hash12(vec2(i, iDate.w)));
		float dist = length(uv-posuv);
       
        if (dist <= radius) {
            #if DIST_OUTSIDE == 1
            	ret = EncodeData(fragCoord, color);      
            #else
            	ret = vec4(0.0);
            #endif
        }
    }
    
    // draw lines
    for (int i = 0; i < c_numLines; ++i)
    {
        vec2 startuv = hash23(vec3(i, iDate.w, 0.453));
        startuv.x *= aspectRatio;
        
        vec2 lineDir = normalize(hash23(vec3(i, iDate.w, 0.627)));
        
        vec3 color = hash33(vec3(i, iDate.w, 2.564));
        float width = mix(c_lineSize.x, c_lineSize.z, hash13(vec3(i, iDate.w, 1.453)));
        float height = mix(c_lineSize.y, c_lineSize.w, hash13(vec3(i, iDate.w, 0.894)));
		vec2 enduv = startuv + lineDir * width;
        
        float dist = UDFatLineSegment(uv, startuv, enduv, height);
        
        if (dist <= 0.0) {
            #if DIST_OUTSIDE == 1
            	ret = EncodeData(fragCoord, color);      
            #else
            	ret = vec4(0.0);
            #endif            
        }
    }    
    
    // draw squiggle lines
    for (int i = 0; i < c_numSquiggles; ++i)
    {
        vec2 startuv = hash23(vec3(i, iDate.w, 2.635));
        startuv.x *= aspectRatio;
        
        vec2 squiggleDir = normalize(hash23(vec3(i, iDate.w, 0.912)));
        
        vec3 color = hash33(vec3(i, iDate.w, 0.123));
        float width = mix(c_squiggleSize.x, c_squiggleSize.z, hash13(vec3(i, iDate.w, 0.342)));
        float height = mix(c_squiggleSize.y, c_squiggleSize.w, hash13(vec3(i, iDate.w, 1.847)));
		vec2 enduv = startuv + squiggleDir * width;
        
        vec2 uvOffset = vec2(0.0);
        float uvdp = dot(uv - startuv, squiggleDir);
        vec2 normal = vec2(squiggleDir.y, -squiggleDir.x);
        if (uvdp >= 0.0 && uvdp <= width)
            uv += normal * sin(uvdp * 100.0) * 0.01;
        
        float dist = UDFatLineSegment(uv, startuv, enduv, height);
        
        if (dist <= 0.0) {
            #if DIST_OUTSIDE == 1
            	ret = EncodeData(fragCoord, color);      
            #else
            	ret = vec4(0.0);
            #endif
        }
    }        

    return ret;
}

//============================================================
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 adjustedFragCoord = fragCoord;
    
    // initialize random seed locations and colors
    if (iFrame == 0) {
        fragColor = DrawShapes(adjustedFragCoord);
        return;
    }
    
    // JFA steps
    fragColor = StepJFA(adjustedFragCoord, floor(float(iFrame-1)));
}
#version 460 core

#define ZERO3 vec3(0.0, 0.0, 0.0)
#define FRONT vec3(1.0, 0.0, 0.0)
#define RIGHT vec3(0.0, 1.0, 0.0)
#define UP vec3(0, 0.0, 1.0)

in vec3 vertColor;
out vec4 FragColor;

uniform mat4 viewMat;
uniform float aspectRatio;
uniform vec3 lightDir;

vec2 opU(vec2 d1, vec2 d2) { return (d1.x < d2.x) ? d1 : d2; }
float sdSphere(vec3 p, float s) { return length(p) - s; }
float sdPlane(vec3 p, vec3 n, float h) { return dot(p, n) + h; }

vec2 map(in vec3 pos)
{
    vec2 res = vec2(1e10, 0.0);
    res = opU(res, vec2(sdSphere(pos - vec3(-1.0, 0.0, 0.0), 0.5), 1.0));
    res = opU(res, vec2(sdSphere(pos - vec3(0.0, 0.0, 0.0), 0.5), 2.0));
    res = opU(res, vec2(sdPlane(pos - vec3(0.0, 0.0, 0.0), UP, .5), 3.0));
    //res = opU(res, vec2(sdSphere(pos, 1.25), 26.0));
    return res;
}

#define SMALL_NUMBER 0.01
vec3 calcNormal(in vec3 pos)
{
#if 0
   // return normalize(map(pos + vec3(SMALL_NUMBER, 0., 0.) + map(pos + vec3(0., SMALL_NUMBER, 0.)),
   //                       pos + vec3(0., 0., SMALL_NUMBER)));
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize( e.xyy*map( pos + e.xyy ).x + 
					  e.yyx*map( pos + e.yyx ).x + 
					  e.yxy*map( pos + e.yxy ).x + 
					  e.xxx*map( pos + e.xxx ).x );
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for (int i = 0; i < 4; i++) {
        vec3 e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
        n += e * map(pos + 0.0005 * e).x;
        //if( n.x+n.y+n.z>100.0 ) break;
    }
    return normalize(n);
#endif
}

#define STEPS 128
#define CLOSE_ENOUGH 0.01
#define FAR 1000.0

float calcSoftshadow(in vec3 ro, in vec3 rd, in float mint, in float tmax)
{
    // bounding volume
    //float tp = (0.8-ro.y)/rd.y; if( tp>0.0 ) tmax = min( tmax, tp );

    float res = 1.0;
    float t = mint;
    for (int i = 0; i < 24; i++) {
        float h = map(ro + rd * t).x;
        float s = clamp(8.0 * h / t, 0.0, 1.0);
        res = min(res, s * s * (3.0 - 2.0 * s));
        t += clamp(h, 0.02, 0.2);
        if (res < 0.004 || t > tmax)
            break;
    }
    return clamp(res, 0.0, 1.0);
}

#define PI 3.1415926535
#define PI3 3.1415926535 / 3.
vec3 remapColor(float a)
{
    vec3 col = vec3(sin(a), sin(a + PI3), sin(a + PI3 * 2));
    return col * col;
}

vec3 castRay(in vec3 ro, in vec3 rd)
{
    vec3 rayPos = ro;
    for (int i = 0; i < STEPS; i++) {
        vec2 rayData = map(rayPos);
        float currentDistance = rayData.x;
        if (currentDistance < CLOSE_ENOUGH) {
            float shadow = calcSoftshadow(rayPos, lightDir, 0.02, 2.5);
            vec3 normal = calcNormal(rayPos);
            vec3 color = remapColor(rayData.y);
            float spec = pow(max(dot(lightDir, reflect(rd, normal)), 0), 20) * shadow;
            return color * (shadow * 0.8 + 0.2) * max(dot(normal, lightDir), 0.0) + vec3(spec);
        } else if (currentDistance > FAR)
            break;
        rayPos += rd * currentDistance;
    }
    return ZERO3;
}

void main()
{
    vec2 uv = vertColor.xy * vec2(aspectRatio, 1.0);
    vec3 rayOrigin = -viewMat[3].xyz;

    vec3 rayDir = normalize(mat3x3(viewMat) * vec3(uv, 1.1));
    vec3 color = castRay(rayOrigin, rayDir);
    FragColor = vec4(color, 1.0);
}

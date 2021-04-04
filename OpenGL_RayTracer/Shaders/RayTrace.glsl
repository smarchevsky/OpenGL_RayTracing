#version 460 core

#define FRONT vec3(1.0, 0.0, 0.0)
#define RIGHT vec3(0.0, 1.0, 0.0)
#define UP vec3(0, 0.0, 1.0)
#define PI 3.1415926535
#define PI3 3.1415926535 / 3.

const float inf = uintBitsToFloat(0x7F800000);
const float ninf = uintBitsToFloat(0xFF800000);
#define MAX 999
in vec3 vertColor;
out vec4 FragColor;
uniform mat4 viewMat;
uniform float aspectRatio;
uniform vec3 lightDir;

uniform vec3 sphPositions[8];

vec3 hash3(vec3 p3)
{
    p3 = fract(p3 * vec3(5.3983, 5.4427, 6.9371));
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.zyx + p3.yxz) * p3.zxy);
}

vec3 sphRand(vec3 p)
{
    vec3 rand = hash3(p);
    float theta = rand.x * 2.0 * PI;
    float phi = (rand.y * 2.0 - 1.0) * PI;
    float r = pow(rand.z, 1.0 / 3.0);
    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);
    return vec3(x, y, z) * r;
}

#define EVALUATE_RAY(k, dist, n, id, newId) \
    if (k > 0 && k < dist) {                \
        dist = k;                           \
        n = newN;                           \
        id = newId;                         \
    }

vec3 rainbow(float a)
{
    vec3 col = vec3(sin(a), sin(a + PI3), sin(a + PI3 * 2));
    return col * col;
}

vec3 sky(vec3 rd)
{
    //return vec3(.2, .3, .34) + vec3(fract(-rd.z)) * .5;
    vec3 col = vec3(0.3, 0.6, 1.0);
    vec3 sun = vec3(0.95, 0.9, 1.0);
    sun *= max(0.0, pow(dot(rd, lightDir), 256.0));
    col *= max(0.0, dot(lightDir, vec3(0, 0, 1)));
    return clamp(col + sun * 10, 0.0, 1.0) * 3;
}

float raySphereIntersect(vec3 r0, vec3 rd, vec3 s0, float sr)
{
    float a = dot(rd, rd);
    vec3 s0_r0 = r0 - s0;
    float b = 2.0 * dot(rd, s0_r0);
    float c = dot(s0_r0, s0_r0) - (sr * sr);
    if (b * b - 4.0 * a * c < 0.0) {
        return MAX;
    }
    return (-b - sqrt((b * b) - 4.0 * a * c)) / (2.0 * a);
}

float sphIntersect(vec3 ro, vec3 rd, float ra) // normal, k
{
    float b = dot(ro, rd);
    float c = dot(ro, ro) - ra * ra;
    float h = b * b - c;
    return (h > 0) ? -b - sqrt(h) : MAX;
}

float planeIntersect(vec3 ro, vec3 rd, vec4 p)
{
    float k = -(dot(ro, p.xyz) + p.w) / dot(rd, p.xyz);
    return k > 0 ? k : MAX;
}

float fresnel(float d)
{
    float fres = 1 - abs(d);
    return fres * fres * fres * 0.9 + 0.1;
}

vec3 getColor(vec3 n, vec3 rd, int id)
{
    //return vec3(.8) * fresnel(dot(-rd, n));
    return vec3(.8);
}

vec4 getSceneColor(inout vec3 ro, inout vec3 rd, vec3 sphBlur)
{
    float dist = MAX;
    int id = -1;
    vec3 n = rd;

    for (int i = 0; i < 8; i++) {
        vec3 sphOrigin = sphPositions[i];
        //float k = raySphereIntersect(ro, rd, sphOrigin, 1.0);
        float k = sphIntersect(ro - sphOrigin, rd, 1.0);
        vec3 pos = ro + rd * k;
        vec3 newN = normalize(pos - sphOrigin);
        int newId = 0;
        EVALUATE_RAY(k, dist, n, id, newId);
    }
    {
        vec4 plInfo = vec4(0, 0, 1, 4);
        float k = planeIntersect(ro, rd, plInfo);
        vec3 newN = plInfo.xyz;
        int newId = 1;
        EVALUATE_RAY(k, dist, n, id, newId);
    }

    n += sphBlur * 0.03;
    vec3 color = (dist >= MAX) ? sky(rd) : getColor(n, rd, id);

    vec3 reflected = reflect(rd, n);
    ro += rd * dist;
    //ro += reflected * 0.001;
    ro += n * 0.0001;
    rd = reflected;
    color += max(n, vec3(0));
    return vec4(color, dist);
}

vec3 traceRay(vec3 ro, vec3 rd, int hash)
{
    vec3 sphBlur = sphRand(rd + vec3(hash * 7.31));
    vec3 col = vec3(1);

    for (int i = 0; i < 12; ++i) {
        vec4 sceneCol = getSceneColor(ro, rd, sphBlur);
        sphBlur.z = -sphBlur.z;
        col *= sceneCol.rgb;
        if (sceneCol.a == MAX)
            return col;
    }

    return vec3(0.1);
}

#define STEPS 30
void main()
{
    vec2 uv = vertColor.xy * vec2(aspectRatio, 1.0);
    vec3 rayOrigin = -viewMat[3].xyz;
    vec3 rayDir = normalize(mat3x3(viewMat) * vec3(uv, 1.1));

    vec3 color = vec3(0);
    for (int i = 0; i < STEPS; ++i)
        color += traceRay(rayOrigin, rayDir, i);
    color /= STEPS;

    color = 1.0 - exp(-1.0 * color);
    FragColor = vec4(color, 1.0);
}

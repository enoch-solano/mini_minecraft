#version 330
// noOp.vert.glsl:
// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer

// Column 0 is Right, 1 is Up, 2 is Forward, 3 is Eye
uniform mat4 u_CameraAttribs;
uniform ivec2 u_ScreenDimensions;

in vec2 fs_UV;

layout(location = 0) out vec4 out_Col;

uniform float u_Time;


const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// Sunset palette
const vec3 sunset[5] = vec3[](vec3(255, 229, 119) / 255.0,
                               vec3(254, 192, 81) / 255.0,
                               vec3(255, 137, 103) / 255.0,
                               vec3(253, 96, 81) / 255.0,
                               vec3(57, 32, 51) / 255.0);

const vec3 daytime[5] = vec3[](vec3(167, 230, 247) / 255.0,
                               vec3(151, 222, 248) / 255.0,
                               vec3(92, 209, 245) / 255.0,
                               vec3(65, 184, 238) / 255.0,
                               vec3(26, 131, 218) / 255.0);

vec3 uvToSky(vec2 uv) {
    if (uv.y < 0.5) {
        return daytime[0];
    }
    else if (uv.y < 0.55) {
        return mix(daytime[0], daytime[1], (uv.y - 0.5) / 0.05);
    }
    else if (uv.y < 0.6) {
        return mix(daytime[1], daytime[2], (uv.y - 0.55) / 0.05);
    }
    else if (uv.y < 0.65) {
        return mix(daytime[2], daytime[3], (uv.y - 0.6) / 0.05);
    }
    else if (uv.y < 0.75) {
        return mix(daytime[3], daytime[4], (uv.y - 0.65) / 0.1);
    }
    return daytime[4];
}

vec2 sphereToUV(vec3 p) {
    float phi = atan(p.z, p.x);
    if (phi < 0) {
        phi += TWO_PI;
    }
    float theta = acos(p.y);
    return vec2(1 - phi / TWO_PI, 1 - theta / PI);
}


vec2 random2( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

vec3 random3( vec3 p ) {
    return fract(sin(vec3(dot(p,vec3(127.1, 311.7, 191.999)),
                          dot(p,vec3(269.5, 183.3, 765.54)),
                          dot(p, vec3(420.69, 631.2,109.21))))
                 *43758.5453);
}

float WorleyNoise2D(vec2 uv)
{
    // Tile the space
    vec2 uvInt = floor(uv);
    vec2 uvFract = fract(uv);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec2 neighbor = vec2(float(x), float(y));

            // Random point inside current neighboring cell
            vec2 point = random2(uvInt + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

            // Compute the distance b/t the point and the fragment
            // Store the min dist thus far
            vec2 diff = neighbor + point - uvFract;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float WorleyNoise3D(vec3 p)
{
    // Tile the space
    vec3 pointInt = floor(p);
    vec3 pointFract = fract(p);

    float minDist = 1.0; // Minimum distance initialized to max.

    // Search all neighboring cells and this cell for their point
    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random point inside current neighboring cell
                vec3 point = random3(pointInt + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(u_Time * 0.01 + 6.2831 * point); // 0 to 1 range

                // Compute the distance b/t the point and the fragment
                // Store the min dist thus far
                vec3 diff = neighbor + point - pointFract;
                float dist = length(diff);
                minDist = min(minDist, dist);
            }
        }
    }
    return minDist;
}


float worley3dFBM(vec3 P) {
    float sum = 0;
    float freq = 4;
    float amp = 0.5;
    for (int i = 0; i < 5; i++) {
        sum += WorleyNoise3D(P * freq) * amp;
        freq *= 2;
        amp *= 0.5;
    }
    return sum;
}

void main()
{
    vec3 eye = u_CameraAttribs[3].xyz;
    vec3 R = u_CameraAttribs[0].xyz;
    vec3 U = u_CameraAttribs[1].xyz;
    vec3 F = u_CameraAttribs[2].xyz;
    // Cast ray
    vec3 ref = eye + F;
    float tan_fovy = tan(45.f * 3.14159f / 360.f);
    vec3 V = tan_fovy * U;
    vec3 H = tan_fovy * R * u_ScreenDimensions.x / float(u_ScreenDimensions.y);
    vec3 p = ref + (fs_UV.x * 2.f - 1.f) * H + (fs_UV.y * 2.f - 1.f) * V;
    vec3 ray_dir = normalize(p - eye);
    // Convert ray into procedural color
//    vec3 rayColor = 0.5f * (ray_dir + vec3(1.f, 1.f, 1.f));
    float noise = worley3dFBM(ray_dir - vec3(u_Time * 0.005f));
    noise = noise * 2.f - 1.f;
    vec2 gradientUV = sphereToUV(ray_dir);
    out_Col = vec4(uvToSky(gradientUV + vec2(noise) * 0.1), 1.f);
}

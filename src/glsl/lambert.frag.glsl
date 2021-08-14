#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform sampler2D u_TerrainTexture;
uniform sampler2D u_SkyTexture;
uniform ivec2 u_ScreenDimensions;

uniform vec3 u_PlayerPos;
uniform float u_Time;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec4 fs_Pos;
in vec4 fs_Nor;
in vec4 fs_LightVec;
in vec2 fs_UV;
in float fs_CosPow;
in float fs_Anim;

out vec4 out_Col; // This is the final output color that you will see on your
// screen for the pixel that is currently being processed.

const vec4 skyColor = vec4(0.37f, 0.74f, 1.0f, 1);

float random1(vec3 p) {
    return fract(sin(dot(p,vec3(127.1, 311.7, 999.999)))
                 *43758.5453);
}


float random1( vec2 p ) {
    return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);
}

float mySmoothStep(float a, float b, float t) {
    t = smoothstep(0, 1, t);
    return mix(a, b, t);
}

float cubicTriMix(vec3 p) {
    vec3 pFract = fract(p);
    float llb = random1(floor(p) + vec3(0,0,0));
    float lrb = random1(floor(p) + vec3(1,0,0));
    float ulb = random1(floor(p) + vec3(0,1,0));
    float urb = random1(floor(p) + vec3(1,1,0));

    float llf = random1(floor(p) + vec3(0,0,1));
    float lrf = random1(floor(p) + vec3(1,0,1));
    float ulf = random1(floor(p) + vec3(0,1,1));
    float urf = random1(floor(p) + vec3(1,1,1));

    float mixLoBack = mySmoothStep(llb, lrb, pFract.x);
    float mixHiBack = mySmoothStep(ulb, urb, pFract.x);
    float mixLoFront = mySmoothStep(llf, lrf, pFract.x);
    float mixHiFront = mySmoothStep(ulf, urf, pFract.x);

    float mixLo = mySmoothStep(mixLoBack, mixLoFront, pFract.z);
    float mixHi = mySmoothStep(mixHiBack, mixHiFront, pFract.z);

    return mySmoothStep(mixLo, mixHi, pFract.y);
}

float fbm(vec3 p) {
    float amp = 0.5;
    float freq = 4.0;
    vec3 sum = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        sum += cubicTriMix(p * freq) * amp;
        amp *= 0.5;
        freq *= 2.0;
    }
    return sum;
}

float bilerpNoise(vec2 uv) {
    vec2 uvFract = fract(uv);
    float ll = random1(floor(uv));
    float lr = random1(floor(uv) + vec2(1,0));
    float ul = random1(floor(uv) + vec2(0,1));
    float ur = random1(floor(uv) + vec2(1,1));

    float lerpXL = mySmoothStep(ll, lr, uvFract.x);
    float lerpXU = mySmoothStep(ul, ur, uvFract.x);

    return mySmoothStep(lerpXL, lerpXU, uvFract.y);
}

float fbm2(vec2 uv) {
    float amp = 0.5;
    float freq = 8.0;
    float sum = 0.0;
    for (int i = 0; i < 6; i++) {
        sum += bilerpNoise(uv * freq) * amp;
        amp *= 0.5;
        freq *= 2.0;
    }
    return sum;
}

float waterHeightField(vec2 uv) {
#if 0
    float x = 0.25 * (cos(uv.x + u_Time) + 1.0);
    float y = 0.25 * (sin(uv.y + u_Time) + 1.0);
    return x + y;
#endif
    return fbm2((uv + vec2(u_Time)) * 0.01);
}

vec3 waterNormalDisplacement(vec2 uv) {
    vec2 dx = vec2(1, 0) * 0.1;
    vec2 dy = vec2(0, 1) * 0.1;
    vec2 grad = vec2(waterHeightField(uv + dx) - waterHeightField(uv - dx),
                     waterHeightField(uv + dy) - waterHeightField(uv - dy));
    grad = grad * 20.f;
    float z = sqrt(1.0 - grad.x * grad.x - grad.y * grad.y);
    return vec3(grad.xy, z);
}

// From PBRT
void coordinateSystem(in vec3 nor, out vec3 tan, out vec3 bit) {
    if (abs(nor.x) > abs(nor.y)) {
        tan = vec3(-nor.z, 0, nor.x);
    }
    else {
        tan = vec3(0, nor.z, -nor.y);
    }
    bit = cross(nor, tan);
}

void main()
{
    vec3 toPlayer = fs_Pos.xyz - u_PlayerPos;

    // Material base color (before shading)
    vec4 diffuseColor = texture(u_TerrainTexture, fs_UV);

    vec3 shadingNormal = normalize(fs_Nor.xyz);

    // Grass
    if (fs_UV.x >= 8.f / 16.f && fs_UV.y >= 13.f / 16.f
            &&fs_UV.x < 9.f / 16.f && fs_UV.y < 14.f / 16.f) {
        vec3 smallGrid = floor(fs_Pos.xyz / 16.f);
        float noise = fbm2(fs_Pos.xz / 128.f);
        vec3 rgb = mix(vec3(0.5, 0.5, 0.25) * diffuseColor.rgb, diffuseColor.rgb, noise);
        diffuseColor = vec4(rgb, 1.f);
    }

    // Water
    if (fs_UV.x >= 14.f / 16.f && fs_UV.y >= 3.f / 16.f &&
       fs_UV.x < 15.f / 16.f && fs_UV.y < 4.f / 16.f) {
        // Deform fs_Nor based on noise
        // Generate a tangent and bitangent to form a local coord system
        // at our fragment
        vec3 tan, bit;
        coordinateSystem(shadingNormal, tan, bit);
        mat3 tangentToWorld = mat3(tan, bit, shadingNormal);
        // Get a new normal direction
        shadingNormal = waterNormalDisplacement(fs_Pos.xz);
        shadingNormal = tangentToWorld * shadingNormal;

//        out_Col = vec4(0.5 * (shadingNormal + vec3(1,1,1)), 1.0);
//        return;
    }

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(shadingNormal, normalize(fs_LightVec.xyz));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.35;

    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.

    diffuseColor.rgb *= lightIntensity;
    float fog_t = smoothstep(0.9, 1.0, min(1.0, length(toPlayer.xz / 192.0)));
    vec2 screenSpaceUVs = gl_FragCoord.xy / vec2(u_ScreenDimensions);
    vec4 skyTextureColor = vec4(texture(u_SkyTexture, screenSpaceUVs).rgb, 1.f);
    diffuseColor = mix(diffuseColor, skyTextureColor, fog_t);

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb, diffuseColor.a);
}

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

void main()
{
    // Material base color (before shading)
    vec4 diffuseColor = texture(u_TerrainTexture, fs_UV);

    vec3 shadingNormal = normalize(fs_Nor.xyz);

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(shadingNormal, normalize(fs_LightVec.xyz));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.35;

    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
    //to simulate ambient lighting. This ensures that faces that are not
    //lit by our point light are not completely black.

    diffuseColor.rgb *= lightIntensity;

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb, diffuseColor.a);
}

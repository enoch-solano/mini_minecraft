#version 330
// noOp.vert.glsl:
// A fragment shader used for post-processing that simply reads the
// image produced in the first render pass by the surface shader
// and outputs it to the frame buffer


in vec2 fs_UV;

layout(location = 0) out vec4 color;

uniform sampler2D u_RenderedTexture;

uniform int u_Time;

void main()
{
    color = texture(u_RenderedTexture, fs_UV);
}

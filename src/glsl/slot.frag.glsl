#version 150

uniform sampler2D u_Texture;

in float fs_Anim;
in vec2 fs_UV;

out vec4 out_Col;

void main() {
    out_Col = texture(u_Texture, fs_UV);
    out_Col.rgb *= fs_Anim; // 0.6 - 1.0 - 1.6
}

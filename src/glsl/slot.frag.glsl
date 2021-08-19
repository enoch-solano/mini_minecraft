#version 150

uniform sampler2D u_InventorySlotTexture;

in float fs_Anim;
in vec2 fs_UV;

out vec4 out_Col;

void main() {
    out_Col = texture(u_InventorySlotTexture, fs_UV);
    out_Col.rgb *= vec3(fs_Anim); // 0.6 - 1.0 - 1.6
//    out_Col.rgb = vec3(1.f);
}

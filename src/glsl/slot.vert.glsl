#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec4 vs_Pos;

in float vs_Anim;
in vec2 vs_UV;

out float fs_Anim;
out vec2 fs_UV;

uniform mat4 u_Model;

void main() {
    fs_Anim = vs_Anim;  // used to shade the slot
    fs_UV = vs_UV;

    gl_Position = vs_Pos;
}

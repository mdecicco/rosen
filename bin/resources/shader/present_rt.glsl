// vertex
#version 330
layout (location = 0) in vec2 vpos;
layout (location = 1) in vec2 vtex;

out vec2 o_tex;

void main() {
    gl_Position = vec4(vpos.x, vpos.y, 0.0, 1.0);
	o_tex = vtex;
};

// fragment
#version 330
uniform sampler2D tex;

in vec2 o_tex;
out vec4 frag_color;

void main() {
    frag_color = texture(tex, o_tex);
}

// vertex
#version 330
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec2 vtex;
layout (location = 2) in mat4 transform;

out vec2 o_tex;
out vec3 o_shirt_color;

layout (std140) uniform u_scene { mat4 transform; mat4 projection; mat4 view_proj; } scene;
layout (std140) uniform u_model { mat4 transform; } model;
layout (std140) uniform u_material { vec3 shirt_tint; } material;

vec3 hsv(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    gl_Position = scene.view_proj * transform * vec4(vpos, 1.0);
    o_tex = vtex;
    o_shirt_color = hsv(material.shirt_tint);
};

// fragment
#version 330

in vec2 o_tex;
in vec3 o_shirt_color;

uniform sampler2D tex;

out vec4 frag_color;

bool similar(vec3 color, vec3 test, float epsilon) {
    return all(greaterThanEqual(color, test - epsilon)) && all(lessThanEqual(color, test + epsilon));
}

vec3 colorize(vec3 original, vec3 newcolor) {
    float gs = (original.r + original.g + original.b) * 0.333333;
    return vec3(gs, gs, gs) * newcolor;
}

vec3 base_color(vec2 texcoord) {
    vec3 color = texture(tex, texcoord).rgb;
    vec3 samples[4];
    vec2 px = vec2(1.0 / 260.0, 1.0 / 144.0);
    samples[0] = texture(tex, texcoord + vec2(-px.x    ,    0       )).rgb;
    samples[1] = texture(tex, texcoord + vec2( 0       ,   -px.y    )).rgb;
    samples[2] = texture(tex, texcoord + vec2( px.x    ,    0       )).rgb;
    samples[3] = texture(tex, texcoord + vec2( 0       ,    px.y    )).rgb;
    
    vec3 avg = color + samples[0] + samples[1] + samples[2] + samples[3];
    if (length(samples[0]) < 0.004 || length(samples[2]) < 0.004 || length(samples[3]) < 0.004) {
        if (length(color) < 0.04 && (1.0 - o_tex.y) > px.y * 2) discard;
    }
    
    avg *= 0.2;
    if (length(avg) < 0.004 || o_tex.y < px.y * 2) discard;
    
    samples[0] = texture(tex, texcoord + vec2(-px.x * 2,    0       )).rgb;
    samples[1] = texture(tex, texcoord + vec2( 0       ,   -px.y * 2)).rgb;
    samples[2] = texture(tex, texcoord + vec2( px.x * 2,    0       )).rgb;
    samples[3] = texture(tex, texcoord + vec2( 0       ,    px.y * 2)).rgb;
    avg *= 0.5;
    avg += (samples[0] + samples[1] + samples[2] + samples[3]) * 0.125;
    if (length(avg) < 0.004) discard;
    
    if (length(samples[0]) < 0.004 || length(samples[2]) < 0.004 || length(samples[3]) < 0.004) {
        if (length(color) < 0.04 && (1.0 - o_tex.y) > px.y * 2) discard;
    }
    
    if (similar(avg, vec3(5.0, 14.0, 31.0) / 255.0, 0.04)) color = colorize(color, o_shirt_color);
    else if (similar(avg, vec3(14.0, 22.0, 50.0) / 255.0, 0.04)) color = colorize(color, o_shirt_color);
    else if (similar(avg, vec3(29.0, 38.0, 63.0) / 255.0, 0.055)) color = colorize(color, o_shirt_color);
    
    return color;
}

void main() {
    frag_color = vec4(base_color(o_tex), 1.0);
}
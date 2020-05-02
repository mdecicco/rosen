// vertex
#version 330
layout (location = 0) in vec3 vpos;
layout (location = 1) in vec3 vnorm;
layout (location = 2) in vec2 vtex;
layout (location = 3) in mat4 transform;
layout (location = 7) in mat3 normal_transform;

out vec2 o_tex;
out vec3 o_norm;
out vec3 o_pos;

layout (std140) uniform u_scene { mat4 view; mat4 projection; mat4 view_proj; } scene;

void main() {
    vec4 wpos = transform * vec4(vpos, 1.0);
    gl_Position = scene.view_proj * wpos;
    o_tex = vec2(vtex.x, 1.0 - vtex.y);
    o_norm = normalize(normal_transform * vnorm);
    o_pos = wpos.xyz;
};

// fragment
#version 330

in vec2 o_tex;
in vec3 o_norm;
in vec3 o_pos;

uniform sampler2D diffuse_tex;
layout (std140) uniform u_material { vec3 diffuse; } material;

uniform int u_light_count;
uniform LightSource u_lights[16];

vec3 calc_light(int index) {
    vec3 L = u_lights[index].position - o_pos;
    float dist = length(L);
    L /= dist;
    vec3 N = normalize(o_norm);
    float NdotL = max(dot(N, L), 0.0);
    float attenuation = 1.0 / (u_lights[index].constantAtt + (u_lights[index].linearAtt * dist) + (u_lights[index].quadraticAtt * dist * dist));
    if (u_lights[index].type == LIGHT_TYPE_DIRECTIONAL) {
    } else if (u_lights[index].type == LIGHT_TYPE_POINT) {
    } else if (u_lights[index].type == LIGHT_TYPE_SPOT) {
        float cos_angle = dot(-L, u_lights[index].direction);
        float inner_minus_outer = u_lights[index].cosConeInnerAngle - u_lights[index].cosConeOuterAngle;
        float spot = clamp((cos_angle - u_lights[index].cosConeOuterAngle) / inner_minus_outer, 0.0, 1.0);
        return u_lights[index].color * NdotL * clamp(spot * attenuation, 0.0, 1.0);
    }

    return vec3(0.0, 0.0, 0.0);
}

out vec4 frag_color;

void main() {
    vec4 diffuse_map = texture(diffuse_tex, o_tex);
    if (diffuse_map.a < 0.05) discard;

    vec3 lighting = vec3(0.0, 0.0, 0.0);
    for (int i = 0;i < u_light_count;i++) lighting += calc_light(i);
    frag_color = vec4(diffuse_map.rgb * lighting, 1.0);
}

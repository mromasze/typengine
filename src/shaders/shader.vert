#version 450
layout(location = 0) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    vec2 pos;
    vec2 size;
    vec2 uvOffset;
    vec2 uvSize;
    vec2 screenSize;
} pc;

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0)
);

void main() {
    vec2 vPos = positions[gl_VertexIndex];
    fragTexCoord = pc.uvOffset + vPos * pc.uvSize;
    
    vec2 worldPos = pc.pos + vPos * pc.size;
    vec2 ndcPos = (worldPos / pc.screenSize) * 2.0 - 1.0;
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
}

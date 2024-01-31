#version 430 core
//layout (location = 0) out vec4 fragColor;
out vec4 fragColor;
in vec2 coord;
uniform sampler2D u_water;
uniform vec2 u_center;
uniform float u_radius;
uniform float u_strength;

void main() {
    const float PI = 3.141592653589793;
    vec2 u_delta=vec2(0.01,0.01);
    vec4 info = texture(u_water, coord);
    
    if(u_center.x>0)
    {
    float drop = max(0.0, 1.0 - length(u_center  - coord) / u_radius);
    drop =0.5-cos(drop * PI)*0.5;
    info.r += drop * u_strength;
    }
    else{
    vec2 dx=vec2(u_delta.x,0.0);
    vec2 dy=vec2(0.0,u_delta.y);
    float average=(
    texture(u_water,coord-dx).r+
    texture(u_water,coord-dy).r+
    texture(u_water,coord+dx).r+
    texture(u_water,coord+dy).r
    )*0.25;

    info.g+=(average-info.r)*2.0;
    info.g*=0.995;
    info.r+=info.g;
    }
    
    fragColor = info;
}
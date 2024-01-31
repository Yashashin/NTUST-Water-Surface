#version 430 core
layout(location = 0) in vec3 position;


out vec2 coord;


void main()
{
    //gl_Position=vec4(vec3(position.x,position.z,position.y),1.0f);
    gl_Position=vec4(position,1.0f);
    coord=position.xy*0.5f+0.5;
}
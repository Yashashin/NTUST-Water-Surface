#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 s_model;

uniform mat4 u_projection;

uniform mat4 u_view;

out vec3 TexCoords;



void main()
{
  gl_Position=u_projection*u_view*s_model*vec4(aPos,1.0);
    TexCoords=aPos;
}
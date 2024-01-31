#version 430 core
layout (location = 0) in vec3 aPos;
layout (location=1) in vec3 aNormal;


uniform mat4 s_model;
uniform mat4 u_projection;
uniform mat4 u_view;

out vec3 TexCoords;
out vec3 Normal;
out vec3 Position;



void main()
{
  Normal=mat3(transpose(inverse(s_model)))*aNormal; //don't know why
  Position=vec3(s_model*vec4(aPos,1.0));
  gl_Position=u_projection*u_view*s_model*vec4(aPos,1.0);
    TexCoords=aPos;
}
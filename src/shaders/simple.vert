#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

uniform mat4 u_model;

uniform float time;

 layout (std140, binding = 0) uniform commom_matrices
 {
     mat4 u_projection;
     mat4 u_view;
 };

out V_OUT
{
    vec3 position;
    vec3 normal;
    vec2 texture_coordinate;
}v_out;



void main()
{
  vec3 pos=position;
  float amplitude=1.0f; 
  float k=30.0;
  float f=k*(pos.x-time);
  vec3 tangent=normalize(vec3(1,(k/30.0)*cos(f),0));
  vec3 newNormal=vec3(-tangent.y,tangent.x,0);
  pos.y=amplitude*sin(f)/30;
  gl_Position = u_projection *u_view * u_model * vec4(pos, 1.0f);

  v_out.position=vec3(u_model*vec4(pos,1.0f));
  v_out.normal=mat3(transpose(inverse(u_model))) * newNormal; 
  v_out.texture_coordinate=vec2(texture_coordinate.x,1.0f-texture_coordinate.y);
}
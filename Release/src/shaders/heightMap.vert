#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

uniform mat4 u_model;
uniform sampler2D heightMap;
uniform sampler2D ripple;
uniform float amplitude;

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
  pos.y=pos.y+amplitude*(texture(heightMap,texture_coordinate).r);//+texture(ripple,texture_coordinate).r*0.5;
 
  gl_Position = u_projection *u_view * u_model * vec4(pos, 1.0f);

  v_out.position=vec3(u_model*vec4(pos,1.0f));
  v_out.normal=mat3(transpose(inverse(u_model))) * normal; 
  v_out.texture_coordinate=vec2(texture_coordinate.x,1.0f-texture_coordinate.y);
}
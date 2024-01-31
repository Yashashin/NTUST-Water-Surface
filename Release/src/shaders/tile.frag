#version 430 core
out vec4 FragColor;

in vec3 TexCoords;
in vec3 Normal;
in vec3 Position;

uniform samplerCube tile;
uniform samplerCube skybox;
uniform sampler2D heightmap;

uniform float amplitude;
uniform vec3 cameraPos;



void main()
{  
    float ratio=1.00/1.52;
     vec3 I=normalize(Position-cameraPos);
    vec3 R1=reflect(I,normalize(Normal));
    vec3 R2=refract(I,normalize(Normal),ratio);
 


  FragColor=texture(tile,TexCoords);
  // FragColor.a=0.5;
  
   
}

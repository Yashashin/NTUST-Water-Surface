#version 430 core
out vec4 f_color;

struct Material{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct DirLight{
    vec3 direction;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight{
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

in V_OUT
{
vec3 position;
vec3 normal;
vec2 texture_coordinate;
}f_in;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform Material material;

vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir);
vec3 CalcPointLight(PointLight light,vec3 normal,vec3 position,vec3 viewDir);

uniform sampler2D u_texture;
uniform sampler2D u_heightMap;
uniform sampler2D u_ripple;
uniform samplerCube tile;
uniform samplerCube skybox;
uniform float f_amplitude;

void main()
{   
    
    vec4 info=texture(u_heightMap,f_in.texture_coordinate);
    float dx=0.001f;
    float dz=0.001f;
    float dy=texture(u_heightMap,vec2(f_in.texture_coordinate.x+dx,f_in.texture_coordinate.y)).r-info.r;
    vec3 du=vec3(dx,dy*f_amplitude,0.0);

    dy=texture(u_heightMap,vec2(f_in.texture_coordinate.x,f_in.texture_coordinate.y+dz)).r-info.r;
    vec3 dv=vec3(0.0,dy*f_amplitude,dz);
    vec3 norm = normalize(cross(dv,du));

    vec3 viewDir = normalize(viewPos - f_in.position-vec3(0,f_amplitude*texture(u_heightMap,f_in.texture_coordinate).r,0));
     vec3 result = vec3(texture(u_texture,f_in.texture_coordinate));
    vec3 dirlight=CalcDirLight(dirLight,norm, viewDir);

     for(int i = 0; i <1; i++)
     {
  //result += CalcPointLight(pointLights[i], norm, f_in.position, viewDir); 
     }
    float ratio=1.0/1.33;
    vec3 I=normalize(f_in.position+vec3(0,f_amplitude*texture(u_heightMap,f_in.texture_coordinate).r,0)-viewPos);
    vec3 R1=reflect(I,normalize(f_in.normal));
    vec3 R2=refract(I,normalize(f_in.normal),ratio);
    R2=normalize(R2);
    float face[5];
    face[0]=(-100-f_amplitude*texture(u_heightMap,f_in.texture_coordinate).r)/R2.y;
    face[1]=(-100-f_in.position.x)/R2.x;
    face[2]=(100-f_in.position.x)/R2.x;
    face[3]=(-100-f_in.position.z)/R2.z;
    face[4]=(100-f_in.position.z)/R2.z;
    float mini=10000;
    for(int i=0;i<5;i++)
    {
     if(face[i]>=0 && face[i]<mini)
     mini=face[i];
    }
    R2=R2*(mini);
   vec3 vector=normalize(R2+f_in.position+vec3(0,f_amplitude*texture(u_heightMap,f_in.texture_coordinate).r,0));
  f_color =mix(mix(vec4(result,1.0),texture(tile,vector),0.6),texture(skybox,R1),0.6)+vec4(dirlight,1.0);
   //f_color = vec4(result,1.0);//+vec4(dirlight,1.0);
    //f_color=texture(u_ripple,f_in.texture_coordinate);
}
vec3 CalcDirLight(DirLight light,vec3 normal,vec3 viewDir)
{
    vec3 lightDir=normalize(-light.direction);
    //Diffuse shading
    float diff=max(dot(normal,lightDir),0.0);
    //Specular shading
    vec3 reflectDir=reflect(-lightDir,normal);
    float spec=pow(max(dot(viewDir,reflectDir),0.0),material.shininess);
    //Combine results
    vec3 ambient = light.ambient*vec3(0.5,0.5,0.5);
    vec3 diffuse=light.diffuse*diff*vec3(0.5,0.5,0.5);
    vec3 specular=light.specular*spec*vec3(2,2,2);
    return (ambient+diffuse+specular);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 position, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - position);
    // Diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // Specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // Attenuation
    float distance = length(light.position - position);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    // Combine results
    vec3 ambient = light.ambient * vec3(0.5,0.5,0.5);
    vec3 diffuse = light.diffuse * diff * vec3(0.8,0.8,0.8);
    vec3 specular = light.specular * spec * vec3(0.8,0.8,0.8);
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
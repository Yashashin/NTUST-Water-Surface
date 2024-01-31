#version 430 core
out vec4 f_color;

struct Material{
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


void main()
{   
    vec3 norm = normalize(f_in.normal);
    vec3 viewDir = normalize(viewPos - f_in.position);
    vec3 result = vec3(texture(u_texture,f_in.texture_coordinate));
    vec3 dirlight=CalcDirLight(dirLight, norm, viewDir);

     for(int i = 0; i <1; i++)
         //result += CalcPointLight(pointLights[i], norm, f_in.position, viewDir); 
    f_color = vec4(result,0.5)+vec4(dirlight,0.5);
    f_color.a=0.8;
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
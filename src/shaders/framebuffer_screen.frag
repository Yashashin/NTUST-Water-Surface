#version 430 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

uniform float screen_w; 
uniform float screen_h; 
uniform bool isPixel;


void main()
{
    float pixel_w=15;
    float pixel_h=10;
    float offset=0.5;
   
     vec3 col = texture(screenTexture, TexCoords).rgb;
     FragColor = vec4(col, 1.0);
    if(isPixel)
    {
      vec2 uv = TexCoords.xy;
            vec3 tc = vec3(1.0, 0.0, 0.0);
            if (uv.x < (offset-0.005))
            {
            float dx = pixel_w*(1./screen_w);
            float dy = pixel_h*(1./screen_h);
            vec2 coord = vec2(dx*floor(uv.x/dx),
                                dy*floor(uv.y/dy));
            tc = texture2D(screenTexture, coord).rgb;
            }
            else if (uv.x>=(offset+0.005))
            {
            tc = texture2D(screenTexture, uv).rgb;
            }
            FragColor = vec4(tc, 1.0);
    }
    else
    {
       vec3 col = texture(screenTexture, TexCoords).rgb;
     FragColor = vec4(col, 1.0);
    }
} 
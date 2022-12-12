#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec2 Pos;

uniform sampler2D screenTexture;

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
    vec3 res = col;
    res.r = res.r * (1-0.8);
    res.b = res.b * (1-(Pos.y) * 0.05);
    res.g = res.g * (1-0.4+(Pos.y)*0.2);
    FragColor = vec4(res, 1.0);
} 
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform bool isBlur;

// Wartoœci offsetu dla prostej próbki 9x9 (Gaussian Blur approximation)
const float offset = 1.0 / 300.0; // Ustawione na przyk³adow¹ wartoœæ
const float blur_weight[9] = float[](0.0, 0.015, 0.05, 0.2, 0.385, 0.2, 0.05, 0.015, 0.0);

void main()
{
    vec4 color = texture(screenTexture, TexCoords);

    if (isBlur)
    {
        vec3 blurred = vec3(0.0);
        // Poziomy Blur (wersja uproszczona, tylko horyzontalna)
        for(int i = 0; i < 9; i++) {
            blurred += texture(screenTexture, TexCoords + vec2(offset * float(i-4), 0.0)).rgb * blur_weight[i];
        }
        color.rgb = blurred;
    }
    
    FragColor = color;
}
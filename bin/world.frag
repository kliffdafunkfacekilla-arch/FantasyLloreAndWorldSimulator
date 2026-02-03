#version 330 core

// Output to screen
out vec4 FragColor;

// Input from Vertex Shader
in vec3 CellColor;

void main() {
    // Standard Rendering (Square Pixels)
    FragColor = vec4(CellColor, 1.0);

    // OPTIONAL: Round Points (Uncomment for circular dots)
    // vec2 coord = gl_PointCoord - vec2(0.5);
    // if(length(coord) > 0.5) discard;
}

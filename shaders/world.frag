#version 330 core
out vec4 FragColor;
in vec3 CellColor; // Passed from Vertex Shader

void main() {
    // We draw each cell as a small circle or dot
    FragColor = vec4(CellColor, 1.0);
}

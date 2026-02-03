#version 330 core
layout(location = 0) in float aX;
layout(location = 1) in float aY;
layout(location = 2) in vec3 aColor;

out vec3 CellColor;

void main() {
    // Basic normalized coords (-1 to 1) 
    // Assuming input X,Y are 0..1? We might need to map them.
    // Let's assume user generation puts them in some range.
    // For now, pass through as clip space or simple map.
    // Map 0..1 to -1..1
    float glX = (aX * 2.0) - 1.0;
    float glY = (aY * 2.0) - 1.0;
    
    gl_Position = vec4(glX, glY, 0.0, 1.0);
    gl_PointSize = 1.0; // 1 pixel dot
    CellColor = aColor;
}

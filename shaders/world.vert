#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec3 aColor;

uniform float u_zoom;
uniform vec2 u_offset;
uniform float u_pointSize;

out vec3 CellColor;

void main() {
    // 1. Apply Offset (move camera)
    vec2 pos = aPos - u_offset;

    // 2. Apply Zoom (scale)
    pos *= u_zoom;

    // 3. Map 0..1 to -1..1 (Clip Space)
    float glX = (pos.x * 2.0) - 1.0;
    float glY = (pos.y * 2.0) - 1.0;
    
    gl_Position = vec4(glX, glY, 0.0, 1.0);
    gl_PointSize = u_pointSize;
    CellColor = aColor;
}

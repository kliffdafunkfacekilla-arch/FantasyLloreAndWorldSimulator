#version 330 core

// Inputs from MapRenderer
layout (location = 0) in vec2 aPos;   // The X,Y coordinate (0.0 to 1.0)
layout (location = 1) in vec3 aColor; // The RGB color we calculated

// Outputs to Fragment Shader
out vec3 CellColor;

// Uniforms from "God Mode" UI
uniform float u_zoom;      // Controlled by "View Level" combo box
uniform vec2 u_offset;     // Controlled by "View Offset" slider
uniform float u_pointSize; // Visual styling

void main() {
    // 1. Apply Offset (Pan)
    // We shift the world so the camera centers on u_offset
    vec2 position = aPos - u_offset;

    // 2. Apply Zoom
    position *= u_zoom;

    // 3. Convert to Clip Space (-1.0 to 1.0) for OpenGL
    // Note: 'position' is currently centered at 0.0 because of the offset subtraction
    // We multiply by 2.0 to span the full screen width/height if zoom is 1.0
    gl_Position = vec4(position.x * 2.0, position.y * 2.0, 0.0, 1.0);

    // 4. Set Point Size
    // High zoom levels need larger points to look like "blocks"
    gl_PointSize = u_pointSize;

    // Pass color to the pixel painter
    CellColor = aColor;
}

#version 150

uniform mat4 modelViewProjectionMatrix;

uniform float uScale;

in vec4 position;
in float density;
in float cellSize;

out float vDensity;

void main()
{
    gl_Position = modelViewProjectionMatrix * position;
	gl_PointSize = cellSize * uScale;

    vDensity = density;
}

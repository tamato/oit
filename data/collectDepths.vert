#version 430

layout(location = 0) in vec4 Position;

layout(location = 0) uniform mat4 ProjectionView;

layout(location = 4) uniform vec4 MoveToOrigin;
layout(location = 5) uniform mat4 RotationMatrix;

layout(location = 0) out float Depth;

void main() {
	vec4 origin = Position - MoveToOrigin;
	origin = RotationMatrix * origin;
	origin += MoveToOrigin;

    gl_Position = ProjectionView * origin;
    Depth = gl_Position.z;
}

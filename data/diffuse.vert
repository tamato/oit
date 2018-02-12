#version 430

layout(location = 0) in vec4 Position;
layout(location = 1) in vec3 Normal;

layout(location = 0) uniform mat4 ProjectionView;

layout(location = 4) uniform vec4 MoveToOrigin;
layout(location = 5) uniform mat4 RotationMatrix;
layout(location = 9) uniform vec4 AnimatedVector;

layout(location = 0) out vec3 outNormal;

void main() {
	vec4 origin = Position - MoveToOrigin;
	origin = RotationMatrix * origin;
	origin += MoveToOrigin;
	
    gl_Position = ProjectionView * (AnimatedVector + origin);
    outNormal = Normal;
}

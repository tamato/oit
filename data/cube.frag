#version 430
#extension GL_ARB_shading_language_420pack: enable  // Use for GLSL versions before 420.

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 EyeVector; // position - camera (going in the neg Z)
layout(location = 3) in vec3 LightDir;

layout(binding=0) uniform sampler2D NormalMap;

layout(location = 1) uniform vec4 Diffuse;
layout(location = 2) uniform float Shininess;

layout(location = 0) out vec4 FragColor;


// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord )
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    vec3 map = texture( NormalMap, texcoord ).xyz;
    map = 2 * map - 1;
    map.z = .7;
    map = normalize(map);
    mat3 TBN = cotangent_frame( N, V, texcoord );
    return normalize( TBN * map );
}

void main() {
#if 0
    vec3 N = texture( NormalMap, TexCoord ).xyz;
    N = 2 * N - 1;
    N.z = .7;
    N = normalize(N);
    vec3 T = normalize(dFdx(EyeVector));
    vec3 B = normalize(cross(N,T));
    T = normalize(cross(B,N));

    mat3 TBN;
    TBN[0] = vec3(T[0],B[0],N[0]);
    TBN[1] = vec3(T[1],B[1],N[1]);
    TBN[2] = vec3(T[2],B[2],N[2]);

    vec3 E =TBN * -normalize(EyeVector);
    vec3 L = TBN * LightDir;
    vec3 H = normalize(E + L);
#else
    vec3 N = perturb_normal(Normal, EyeVector, TexCoord);
    vec3 E = -normalize(EyeVector);
    vec3 L = LightDir;
    vec3 H = normalize(E + L);
#endif

    float df = max(0, dot(L, N));
    float sf = max(0, dot(H, N));
    sf = pow(sf, Shininess);

    // check for self shadowing, the 8 is a number found from trail and error
    // http://developer.download.nvidia.com/assets/gamedev/docs/GDC2K_gpu_bump.pdf
    float ssdf = min(8 * max(0, dot(Normal, L)), 1);
    float sssf = min(8 * max(0, dot(Normal, H)), 1);

    FragColor = (ssdf * df * Diffuse) + (sssf * sf * vec4(1,1,1,1));
}
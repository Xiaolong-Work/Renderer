#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) out vec4 out_ssao;

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput position;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput normal_depth;

layout(binding = 2) uniform CameraMVP 
{
    mat4 model;
    mat4 view;
    mat4 project;
} camera;

layout(binding = 3) uniform sampler2D noise;

// SSAO ����
const int SAMPLE_COUNT = 64;
const float RADIUS = 0.5;
const float POWER = 2.0;

// Ԥ���ɵİ��������
vec3 samples[SAMPLE_COUNT];

void main(){}

//void initializeSamples() 
//{
//    for(int i = 0; i < SAMPLE_COUNT; ++i) 
//	{
//        // �ڰ������������������
//        vec3 sample = vec3(
//            random() * 2.0 - 1.0,
//            random() * 2.0 - 1.0,
//            random()
//        );
//        sample = normalize(sample);
//        
//        // ʹ������ƫ���߷���
//        sample *= random();
//        
//        // ���ݾ������Ų�����
//        float scale = float(i) / float(SAMPLE_COUNT);
//        scale = mix(0.1, 1.0, scale * scale);
//        sample *= scale;
//        
//        samples[i] = sample;
//    }
//}

//void main() 
//{
//    // ��ʼ�������� (ʵ��Ӧ����Ӧ���ڳ�������ʱԤ����)
//    initializeSamples();
//    
//    // ��ȡ��ǰ���ص�����ռ�λ�úͷ���
//    vec4 world_position = subpassLoad(position);
//    vec3 fragPos = vec3(world_position / world_position.w);
//    vec3 normal = normalize(subpassLoad(normal_depth).xyz);
//    float linear_depth = subpassLoad(normal_depth).w;
//    
//    // ��ȡ��Ļ�ռ����� (���ڲ�����������)
//    ivec2 texCoord = ivec2(gl_FragCoord.xy);
//    vec2 noiseScale = vec2(textureSize(noise, 0)) / vec2(textureSize(position, 0));
//    vec3 randomVec = texture(noise, vec2(texCoord) * noiseScale).xyz;
//    
//    // ����TBN���� (����->����ռ�)
//    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
//    vec3 bitangent = cross(normal, tangent);
//    mat3 TBN = mat3(tangent, bitangent, normal);
//    
//    // ���㻷���ڱ�
//    float occlusion = 0.0;
//    for(int i = 0; i < SAMPLE_COUNT; ++i) 
//	{
//        // ������������߿ռ�ת��������ռ�
//        vec3 samplePos = TBN * samples[i];
//        samplePos = fragPos + samplePos * RADIUS;
//        
//        // ��������ͶӰ����Ļ�ռ�
//        vec4 offset = vec4(samplePos, 1.0);
//        offset = camera.project * camera.view * offset;
//        offset.xyz /= offset.w;
//        offset.xyz = offset.xyz * 0.5 + 0.5;
//        
//        // ��ȡ�������ʵ�����
//        float sampleDepth = subpassLoad(position, offset.xy).w;
//        
//        // ��Χ����ƽ��
//        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(fragPos.z - sampleDepth));
//        occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;
//    }
//    
//    // ��һ����Ӧ���ݺ�����ǿЧ��
//    occlusion = 1.0 - (occlusion / SAMPLE_COUNT);
//    occlusion = pow(occlusion, POWER);
//    
//    out_ssao = vec4(occlusion, occlusion, occlusion, 1.0);
//}
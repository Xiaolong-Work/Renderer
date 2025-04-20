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

// SSAO 参数
const int SAMPLE_COUNT = 64;
const float RADIUS = 0.5;
const float POWER = 2.0;

// 预生成的半球采样点
vec3 samples[SAMPLE_COUNT];

void main(){}

//void initializeSamples() 
//{
//    for(int i = 0; i < SAMPLE_COUNT; ++i) 
//	{
//        // 在半球内生成随机采样点
//        vec3 sample = vec3(
//            random() * 2.0 - 1.0,
//            random() * 2.0 - 1.0,
//            random()
//        );
//        sample = normalize(sample);
//        
//        // 使采样点偏向法线方向
//        sample *= random();
//        
//        // 根据距离缩放采样点
//        float scale = float(i) / float(SAMPLE_COUNT);
//        scale = mix(0.1, 1.0, scale * scale);
//        sample *= scale;
//        
//        samples[i] = sample;
//    }
//}

//void main() 
//{
//    // 初始化采样点 (实际应用中应该在程序启动时预计算)
//    initializeSamples();
//    
//    // 获取当前像素的世界空间位置和法线
//    vec4 world_position = subpassLoad(position);
//    vec3 fragPos = vec3(world_position / world_position.w);
//    vec3 normal = normalize(subpassLoad(normal_depth).xyz);
//    float linear_depth = subpassLoad(normal_depth).w;
//    
//    // 获取屏幕空间坐标 (用于采样噪声纹理)
//    ivec2 texCoord = ivec2(gl_FragCoord.xy);
//    vec2 noiseScale = vec2(textureSize(noise, 0)) / vec2(textureSize(position, 0));
//    vec3 randomVec = texture(noise, vec2(texCoord) * noiseScale).xyz;
//    
//    // 创建TBN矩阵 (切线->世界空间)
//    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
//    vec3 bitangent = cross(normal, tangent);
//    mat3 TBN = mat3(tangent, bitangent, normal);
//    
//    // 计算环境遮蔽
//    float occlusion = 0.0;
//    for(int i = 0; i < SAMPLE_COUNT; ++i) 
//	{
//        // 将采样点从切线空间转换到世界空间
//        vec3 samplePos = TBN * samples[i];
//        samplePos = fragPos + samplePos * RADIUS;
//        
//        // 将采样点投影到屏幕空间
//        vec4 offset = vec4(samplePos, 1.0);
//        offset = camera.project * camera.view * offset;
//        offset.xyz /= offset.w;
//        offset.xyz = offset.xyz * 0.5 + 0.5;
//        
//        // 获取采样点的实际深度
//        float sampleDepth = subpassLoad(position, offset.xy).w;
//        
//        // 范围检查和平滑
//        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(fragPos.z - sampleDepth));
//        occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;
//    }
//    
//    // 归一化并应用幂函数增强效果
//    occlusion = 1.0 - (occlusion / SAMPLE_COUNT);
//    occlusion = pow(occlusion, POWER);
//    
//    out_ssao = vec4(occlusion, occlusion, occlusion, 1.0);
//}
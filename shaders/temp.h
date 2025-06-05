/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>
// GLSL Type
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

// clang-format off
#ifdef __cplusplus // Descriptor binding helper for C++ and GLSL
 #define START_BINDING(a) enum a {
 #define END_BINDING() }
#else
 #define START_BINDING(a)  const uint
 #define END_BINDING() 
#endif

START_BINDING(SceneBindings)
  eGlobals  = 0,  // Global uniform containing camera matrices
  eObjDescs = 1,  // Access to the object descriptions
  eTextures = 2   // Access to textures
END_BINDING();

START_BINDING(RtxBindings)
  eTlas     = 0,  // Top-level acceleration structure
  eOutImage = 1   // Ray tracer output image
END_BINDING();
// clang-format on

// Information of a obj model when referenced in a shader
struct ObjDesc
{
	int txtOffset;				   // Texture index offset in the array of textures
	uint64_t vertexAddress;		   // Address of the Vertex buffer
	uint64_t indexAddress;		   // Address of the index buffer
	uint64_t materialAddress;	   // Address of the material buffer
	uint64_t materialIndexAddress; // Address of the triangle material index buffer
};

// Uniform buffer set at each frame
struct GlobalUniforms
{
	mat4 viewProj;	  // Camera view * projection
	mat4 viewInverse; // Camera inverse view matrix
	mat4 projInverse; // Camera inverse projection matrix
};

// Push constant structure for the raster
struct PushConstantRaster
{
	mat4 modelMatrix; // matrix of the instance
	vec3 lightPosition;
	uint objIndex;
	float lightIntensity;
	int lightType;
};

// Push constant structure for the ray tracer
struct PushConstantRay
{
	vec3 position;
	int image_index;

	vec3 look;
	int frame;

	vec3 up;
	int pad;
};

struct WaveFrontMaterial // See ObjLoader, copy of MaterialObj, could be compressed for device
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 transmittance;
	vec3 emission;
	float shininess;
	float ior;		// index of refraction
	float dissolve; // 1 == opaque; 0 == fully transparent
	int illum;		// illumination model (see http://www.fileformat.info/format/material/)
	int textureId;
};

#endif

uint tea(uint val0, uint val1)
{
	uint v0 = val0;
	uint v1 = val1;
	uint s0 = 0;

	for (uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}

	return v0;
}

// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg(inout uint prev)
{
	uint LCG_A = 1664525u;
	uint LCG_C = 1013904223u;
	prev = (LCG_A * prev + LCG_C);
	return prev & 0x00FFFFFF;
}

// Generate a random float in [0, 1) given the previous RNG state
float rnd(inout uint prev)
{
	return (float(lcg(prev)) / float(0x01000000));
}

/*
 * Copyright (c) 2024-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
typedef glm::mat4 mat4;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
#endif

// general behavior defines
// benchmark mode - change render load and print perf data
#define BENCHMARK_MODE 0  // default 0
// measure times of the different stages (currently not mgpu-safe!)
#define DEBUG_MEASURETIME 0  // default 0
// exit after a set time in seconds
#define DEBUG_EXITAFTERTIME 0  // default 0

// scene data defines
#define VERTEX_POS 0
#define VERTEX_NORMAL 1

#define UBO_SCENE 1
#define UBO_OBJECT 2

#define MAX_VIEWS 4

// Uniform location for the variable that contains the view each pass in the
// software fallback uses.
#define OFFSET_FALLBACK_ID 2

#ifdef __cplusplus
namespace vertexload {
#endif

struct ObjectData
{
  mat4 model;          // model -> world
  mat4 modelView;      // model -> view
  mat4 modelViewIT;    // model -> view for normals
  mat4 modelViewProj;  // model -> proj
  vec3 color;          // model color
};


struct SceneDataMVR
{
  mat4 viewMatrix[MAX_VIEWS];      // view matrix: world->view
  mat4 projMatrix[MAX_VIEWS];      // proj matrix: view ->proj
  mat4 viewProjMatrix[MAX_VIEWS];  // viewproj   : world->proj
  vec4 eyepos_world[MAX_VIEWS];    // eye position in world space
  vec4 lightPos_world;             // light position in world space

  float torusScale;
  float projNear;
  float projFar;

  int fragmentLoadFactor;
};


#ifdef __cplusplus
}
#endif

#if defined(GL_core_profile) || defined(GL_compatibility_profile) || defined(GL_es_profile)
// prevent this to be used by c++

layout(std140, binding = UBO_SCENE) uniform sceneBuffer
{
  SceneDataMVR scene;
};

layout(std140, binding = UBO_OBJECT) uniform objectBuffer
{
  ObjectData object;
};

#endif

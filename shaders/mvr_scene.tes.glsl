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

#version 450

/*
 * This Tessellation Shader is just present to demonstrate that
 * both, Single-Pass-Stereo and Multi-View Rendering
 * do support Tessellation Shaders (if the right extensions are
 * exposed).
 *
 * This sample shader will tessellatate the input and deform it based on 3D
 * noise in the normal direction.
 */

#extension GL_ARB_shading_language_include : enable

#if defined(STEREO_SPS)
//////////// SinglePassStereo ////////////
// Single Pass Stereo
// - require/enable the extension
// - set the secondary view offset to 1
//
#extension GL_NV_stereo_view_rendering : require
layout(secondary_view_offset = 1) out highp int gl_Layer;
#endif

#if defined(STEREO_MVR)
//////////// SinglePassStereo ////////////
// Multi-View Rendering / GL_OVR_multiview
// - require/enable the extension
// - define the number of views to be rendered
//
#extension GL_EXT_multiview_tessellation_geometry_shader : require
layout(num_views = MVR_VIEWS) in;
#endif

#include "common.h"
#include "noise.glsl"

layout(triangles, equal_spacing, ccw) in;

layout(location = OFFSET_FALLBACK_ID) uniform int fallbackViewID;

in Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
IN[];

out Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
OUT;


vec4 interpolate4(in vec4 v0, in vec4 v1, in vec4 v2)
{
  return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

vec3 interpolate3(in vec3 v0, in vec3 v1, in vec3 v2)
{
  return gl_TessCoord.x * v0 + gl_TessCoord.y * v1 + gl_TessCoord.z * v2;
}

void main()
{
#if defined(STEREO_SPS)
  int viewID = 0;
#elif defined(STEREO_MVR)
  int viewID = int(gl_ViewID_OVR);
#else
  int viewID = fallbackViewID;
#endif

  OUT.normal   = interpolate3(IN[0].normal, IN[1].normal, IN[2].normal);
  OUT.eyeDir   = interpolate3(IN[0].eyeDir, IN[1].eyeDir, IN[2].eyeDir);
  OUT.lightDir = interpolate3(IN[0].lightDir, IN[1].lightDir, IN[2].lightDir);

  vec4 worldPos = interpolate4(IN[0].worldPos, IN[1].worldPos, IN[2].worldPos);

  //
  // Move the vertex a bit based on 3D noise to see an effect of the
  // Tessellation Shader. All lighting parameters would have to be
  // updated as well, but we ignore it here.
  //
  vec3 normal = normalize(OUT.normal);

  float scale     = scene.torusScale;
  float frequency = 1.0 / scale;
  float amplitude = 0.1 * scale;
  float noise     = SimplexPerlin3D(frequency * worldPos.xyz);
  vec3  v         = amplitude * normal * noise;

  float frequency2 = 7.0 / scale;
  float amplitude2 = 0.02 * scale;
  noise            = SimplexPerlin3D(frequency2 * worldPos.xyz);
  v                = v + amplitude2 * normal * noise;

  worldPos.xyz = worldPos.xyz + v;
  gl_Position  = scene.viewProjMatrix[viewID] * worldPos;

#if defined(STEREO_SPS)
  gl_SecondaryPositionNV = scene.viewProjMatrix[1] * worldPos;
#endif

  OUT.worldPos = worldPos;
}

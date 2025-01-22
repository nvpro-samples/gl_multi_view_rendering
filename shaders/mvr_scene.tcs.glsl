/*
 * Copyright (c) 2024, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION
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

layout(vertices = 3) out;

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
OUT[];

void main(void)
{
  // fixed tessellation factors for this simple sample:
  gl_TessLevelOuter[0] = 4.0;
  gl_TessLevelOuter[1] = 4.0;
  gl_TessLevelOuter[2] = 4.0;

  gl_TessLevelInner[0] = 4.0;
  gl_TessLevelInner[1] = 4.0;


  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
#if defined(STEREO_SPS)
  gl_out[gl_InvocationID].gl_SecondaryPositionNV = gl_in[gl_InvocationID].gl_SecondaryPositionNV;
#endif

  OUT[gl_InvocationID].normal   = IN[gl_InvocationID].normal;
  OUT[gl_InvocationID].eyeDir   = IN[gl_InvocationID].eyeDir;
  OUT[gl_InvocationID].lightDir = IN[gl_InvocationID].lightDir;
  OUT[gl_InvocationID].worldPos = IN[gl_InvocationID].worldPos;
}

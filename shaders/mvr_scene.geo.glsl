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
 * This Geometry Shader is just present to demonstrate that
 * both, Single-Pass-Stereo and Multi-View Rendering
 * do support Geometry Shaders (if the right extensions are
 * exposed).
 *
 * This sample shader will generate a 2-triangle arrow on each
 * input triangle.
 */

#extension GL_ARB_shading_language_include : enable

#if defined(STEREO_SPS)
//////////// SinglePassStereo ////////////
// Single Pass Stereo
// - require/enable the extension
// - set the secondary view offset to 1
//
#extension GL_NV_stereo_view_rendering : enable
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

layout(triangles) in;
layout(triangle_strip) out;
// 1 input triangle + 2 new output triangles which represent the arrow in normal direction:
layout(max_vertices = 9) out;

layout(location = OFFSET_FALLBACK_ID) uniform int fallbackViewID;

in Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
vertices[];

out Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
frag;

void main()
{
#if defined(STEREO_SPS)
  int viewID = 0;
#elif defined(STEREO_MVR)
  int viewID = int(gl_ViewID_OVR);
#else
  int viewID = fallbackViewID;
#endif

  // pass through original triangle:
  for(int i = 0; i < 3; ++i)
  {
    frag.normal   = vertices[i].normal;
    frag.eyeDir   = vertices[i].eyeDir;
    frag.lightDir = vertices[i].lightDir;
    frag.worldPos = vertices[i].worldPos;
    gl_Position   = gl_in[i].gl_Position;

#if defined(STEREO_SPS)
    gl_SecondaryPositionNV = gl_in[i].gl_SecondaryPositionNV;
#endif

    EmitVertex();
  }
  EndPrimitive();

  //
  // calculate two new triangles which will be the "normal" of the
  // input triangle
  //
  vec3 pos[3];
  pos[0] = vertices[0].worldPos.xyz / vertices[0].worldPos.w;
  pos[1] = vertices[1].worldPos.xyz / vertices[1].worldPos.w;
  pos[2] = vertices[2].worldPos.xyz / vertices[2].worldPos.w;

  vec3 a      = pos[1] - pos[0];
  vec3 b      = pos[2] - pos[0];
  vec3 center = pos[0] + 0.333 * a + 0.333 * b;

  // scale of the arrow representing the normal should be roughly proportional to the input triangle size:
  float scale = 0.5 * max(length(a), length(b));

  vec3 normal = cross(a.xyz, b.xyz);
  normal      = normalize(normal);

  vec3 tangent = 0.1 * normalize((pos[0] - center));
  vec3 arrow[6];

  arrow[0] = vec3(0.0);
  arrow[1] = tangent + normal;
  arrow[2] = -tangent + normal;

  arrow[3] = -2.0 * tangent + normal;
  arrow[4] = 2.0 * tangent + normal;
  arrow[5] = 1.25 * normal;

  for(int i = 0; i < 6; ++i)
  {
    arrow[i] *= scale;
  }

  // output arrow for the normal:
  for(int t = 0; t < 2; ++t)
  {
    for(int v = 0; v < 3; ++v)
    {
      frag.normal   = vec3(0.0, 0.0, 1.0);
      frag.eyeDir   = vec3(0.0, 0.0, 1.0);
      frag.lightDir = vec3(0.0, 0.0, 1.0);
      frag.worldPos = vec4(center, 1.0);

      vec4 pos    = vec4(center + arrow[3 * t + v], 1.0);
      gl_Position = scene.viewProjMatrix[viewID] * pos;

#if defined(STEREO_SPS)
      gl_SecondaryPositionNV = scene.viewProjMatrix[1] * pos;
#endif

      EmitVertex();
    }
    EndPrimitive();
  }
}

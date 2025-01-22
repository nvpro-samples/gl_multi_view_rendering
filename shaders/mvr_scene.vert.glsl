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

#extension GL_ARB_shading_language_include : enable
#extension GL_NV_viewport_array2 : require

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
#extension GL_OVR_multiview : require
layout(num_views = MVR_VIEWS) in;
#endif

#include "common.h"

// inputs in model space
in layout(location = VERTEX_POS) vec3 vertex_pos_model;
in layout(location = VERTEX_NORMAL) vec3 normal;

layout(location = OFFSET_FALLBACK_ID) uniform int fallbackViewID;

// outputs in view space
out Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
OUT;

void main()
{
  //////////// SinglePassStereo ////////////
  //
  // Using a viewID to pick the right matrices
  // where viewID is based on:
  // * a uniform (fallbackViewID) for our no-extension fallback
  // * a constant for Single Pass Stereo (and adding 1 for the gl_SecondaryPositionNV)
  // * the build-in gl_ViewID_OVR for Multi-View Rendering
  // * if VR SLI (GL_NV_multicast) should also get supported, the viewID would come from a
  //   uniform which was set differently per GPU
  //
  int viewID;
#if defined(STEREO_SPS)
  viewID = 0;
#elif defined(STEREO_MVR)
  viewID = int(gl_ViewID_OVR);
#else
  viewID = fallbackViewID;
#endif


#if defined(STEREO_MVR) && (MVR_VIEWS == 2)
  //////////// SinglePassStereo ////////////
  //
  // The 2-view example allows comparing SPS with MVR
  // and also shows the most common use-case for VR: two views
  // which only differ on the X-axis.
  // SPS implicitly only sets a new X value for the second view,
  // for MVR it is done here explicitly. Keeping the number of
  // variables which depend on the viewID to a minimum helps
  // HW implementations to get the optimal performance.
  //

  mat4 modelViewProjection = scene.viewProjMatrix[0] * object.model;
  gl_Position              = modelViewProjection * vec4(vertex_pos_model, 1);

  if(viewID == 1)
  {
    mat4 modelViewProjection2 = scene.viewProjMatrix[1] * object.model;
    vec4 pos                  = modelViewProjection2 * vec4(vertex_pos_model, 1);
    gl_Position.x             = pos.x;
  }

#else
  //////////// SinglePassStereo ////////////
  //
  // When not using MVR or if MVR has to deal with completely independent
  // view / projection matrices (as shown in the 4 view example), the full
  // projection needs to get calculated.
  //
  mat4 modelViewProjection = scene.viewProjMatrix[viewID] * object.model;
  gl_Position              = modelViewProjection * vec4(vertex_pos_model, 1);
#endif

#if defined(STEREO_SPS)
  //////////// SinglePassStereo ////////////
  //
  // Single Pass Stereo / GL_NV_stereo_view_rendering
  // - write the secondary position based on the second view
  //   result is only allowed to differ in the X value!
  // - set gl_Layer to 0 -> output to layers 0 and 1
  //
  modelViewProjection    = scene.viewProjMatrix[viewID + 1] * object.model;
  gl_SecondaryPositionNV = modelViewProjection * vec4(vertex_pos_model, 1);
  gl_Layer               = 0;
#endif

  //////////// SinglePassStereo ////////////
  //
  // Lighting will get calculated in the view space of view 0
  // to keep the number of view dependent varyings low.
  // This is not a limitation but a performance improvement.
  //
  mat4 modelView = scene.viewMatrix[0] * object.model;

  // view space calculations
  vec3 pos         = (modelView * vec4(vertex_pos_model, 1)).xyz;
  vec3 lightPos    = (scene.viewMatrix[0] * scene.lightPos_world).xyz;
  vec3 eyePos_view = (scene.viewMatrix[0] * vec4(scene.eyepos_world[0])).xyz;

  // We assume that there are no shearings in the model view,
  // so the model view matrix is equal to its inverse transposed:
  OUT.normal   = (modelView * vec4(normal, 0)).xyz;
  OUT.eyeDir   = eyePos_view - pos;
  OUT.lightDir = lightPos - pos;

  OUT.worldPos = object.model * vec4(vertex_pos_model, 1);
}

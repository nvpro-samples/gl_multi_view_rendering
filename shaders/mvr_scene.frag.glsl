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

#extension GL_ARB_shading_language_include : enable
#include "common.h"
#include "noise.glsl"

// inputs in view space
in Interpolants
{
  vec4 worldPos;
  vec3 normal;
  vec3 eyeDir;
  vec3 lightDir;
}
IN;

layout(location = 0, index = 0) out vec4 out_Color;

float calcNoise(vec3 modelPos, int iterations)
{  
  float val = 0;
  for ( int i = 0; i < iterations; ++i )
  {
    val += SimplexPerlin3D(modelPos*20) / iterations;
  }
  val = smoothstep(-0.1, 0.1, val);
  return val;
}

vec4 calculateLight(vec3 normal, vec3 eyeDir, vec3 lightDir, vec3 objColor) 
{
  // ambient term
  vec4 ambient_color = vec4( objColor * 0.25, 1.0 );
  
  // diffuse term
  float diffuse_intensity = max(dot(normal, lightDir), 0.0)/1.5;
  vec4  diffuse_color = diffuse_intensity * vec4(objColor, 1.0);
  
  // specular term
  vec3  R = reflect( -lightDir, normal );
  float specular_intensity = max( dot( eyeDir, R ), 0.0 );
  vec4  specular_color = pow(specular_intensity, 10) * vec4(0.8,0.8,0.8,1);
  
  return ambient_color + diffuse_color + specular_color;
}

void main()
{
  // interpolated inputs in view space
  vec3 normal   = normalize(IN.normal);
  vec3 eyeDir   = normalize(IN.eyeDir);
  vec3 lightDir = normalize(IN.lightDir);

  // scene.fragmentLoadFactor
  vec3 pos = IN.worldPos.xyz/IN.worldPos.w;
  float noiseVal = calcNoise(pos*10, scene.fragmentLoadFactor);
  vec3 objColor = object.color + vec3(noiseVal);

  out_Color = calculateLight(normal, eyeDir, lightDir, objColor);
}

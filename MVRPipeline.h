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

#pragma once

#include "Pipeline.h"
#include "MVRSettings.h"

#include <glm/glm.hpp>
#include "common.h"

#include "nvgl/base_gl.hpp"

#include <string>

class MVRPipeline : public Pipeline<vertexload::SceneDataMVR, vertexload::ObjectData>
{
public:
  MVRPipeline();
  ~MVRPipeline();

  void setObjectColor(const glm::vec3& color) { m_objectColor = color; }

  void setSettings(struct MVRSettings settings);

  void updateObjectUniforms() override;

  // set after the hardware support has been checked:
  bool supportSPS                              = false;
  bool supportMVR                              = false;
  bool supportMVR_texture_multisample          = false;
  bool supportMVR_timer_query                  = false;
  bool supportMVR_tessellation_geometry_shader = false;

private:
  // All programs come from the same scene.*.glsl shader files but are compiled with
  // different defines depending on which hardware feature should be used.
  // The benefit is that it becomes obvious how little change is required to support Single Pass Stereo and Multi-View Rendering.
  struct PipelineVariants
  {
    // simple shaders for all render modes:
    nvgl::ProgramID VS;
    nvgl::ProgramID VS_GS;
    nvgl::ProgramID VS_TS;
    nvgl::ProgramID VS_TS_GS;
  };

  void initShaders(PipelineVariants& progs, const std::string& defines, bool excludeTSandGS = false);

  struct Programs
  {
    PipelineVariants software;
    PipelineVariants sps;
    PipelineVariants mvr;
    PipelineVariants mvr_quad;
  };

  Programs m_programs;

  glm::vec3 m_objectColor;

  struct MVRSettings m_settings;
};

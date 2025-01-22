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

#include "MVRPipeline.h"
#include "MVRDemo.h"

#include "nvh/nvprint.hpp"

MVRPipeline::MVRPipeline()
    : Pipeline<vertexload::SceneDataMVR, vertexload::ObjectData>(UBO_SCENE, UBO_OBJECT)
{
  // check hardware support
  // the next extension is not known to GLEW, so test for it manually:
  GLint numExtensions;
  glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
  for(GLint i = 0; i < numExtensions; ++i)
  {
    std::string name((const char*)glGetStringi(GL_EXTENSIONS, i));
    if(name == "GL_NV_stereo_view_rendering")
    {
      supportSPS = true;
    }
    if(name == "GL_OVR_multiview2")
    {
      // GL_OVR_multiview2 requires GL_OVR_multiview
      supportMVR = true;
    }
    if(name == "GL_EXT_multiview_texture_multisample")
    {
      supportMVR_texture_multisample = true;
    }
    if(name == "GL_EXT_multiview_tessellation_geometry_shader")
    {
      supportMVR_tessellation_geometry_shader = true;
    }
    if(name == "GL_EXT_multiview_timer_query")
    {
      supportMVR_timer_query = true;
    }
  }

  LOGOK("\nGL_NV_stereo_view_rendering extension %sfound!\n", supportSPS ? "" : "NOT ");
  LOGOK("\nGL_OVR_multiview & GL_OVR_multiview2 extensions %sfound!\n", supportMVR ? "" : "NOT ");
  LOGOK("\nGL_EXT_multiview_texture_multisample extension %sfound!\n", supportMVR_texture_multisample ? "" : "NOT ");
  LOGOK("\nGL_EXT_multiview_tessellation_geometry_shader extension %sfound!\n", supportMVR_tessellation_geometry_shader ? "" : "NOT ");
  LOGOK("\nGL_EXT_multiview_timer_query extension %sfound!\n", supportMVR_timer_query ? "" : "NOT ");


  // init shaders
  m_progManager.registerInclude("common.h", "common.h");

  initShaders(m_programs.software, "");
  initShaders(m_programs.sps, "#define STEREO_SPS\n");
  if(supportMVR)
  {
    initShaders(m_programs.mvr, "#define STEREO_MVR\n#define MVR_VIEWS 2\n", !supportMVR_tessellation_geometry_shader);
    initShaders(m_programs.mvr_quad, "#define STEREO_MVR\n#define MVR_VIEWS 4\n", !supportMVR_tessellation_geometry_shader);
  }

  bool valid = m_progManager.areProgramsValid();
  if(!valid)
  {
    LOGE("Error loading shader files\n");
  }

  m_program = m_programs.software.VS;
}

void MVRPipeline::initShaders(PipelineVariants& progs, const std::string& defines, bool excludeTSandGS)
{
  std::string generalDefines = "#define USE_MVR_SCENE_DATA\n";

  std::string allDefines = generalDefines + defines;
  progs.VS =
      m_progManager.createProgram(nvgl::ProgramManager::Definition(GL_VERTEX_SHADER, allDefines, "mvr_scene.vert.glsl"),
                                  nvgl::ProgramManager::Definition(GL_FRAGMENT_SHADER, allDefines, "mvr_scene.frag.glsl"));

  if(excludeTSandGS)
    return;

  progs.VS_GS =
      m_progManager.createProgram(nvgl::ProgramManager::Definition(GL_VERTEX_SHADER, allDefines, "mvr_scene.vert.glsl"),
                                  nvgl::ProgramManager::Definition(GL_GEOMETRY_SHADER, allDefines, "mvr_scene.geo.glsl"),
                                  nvgl::ProgramManager::Definition(GL_FRAGMENT_SHADER, allDefines, "mvr_scene.frag.glsl"));

  progs.VS_TS =
      m_progManager.createProgram(nvgl::ProgramManager::Definition(GL_VERTEX_SHADER, allDefines, "mvr_scene.vert.glsl"),
                                  nvgl::ProgramManager::Definition(GL_TESS_CONTROL_SHADER, allDefines, "mvr_scene.tcs.glsl"),
                                  nvgl::ProgramManager::Definition(GL_TESS_EVALUATION_SHADER, allDefines, "mvr_scene.tes.glsl"),
                                  nvgl::ProgramManager::Definition(GL_FRAGMENT_SHADER, allDefines, "mvr_scene.frag.glsl"));

  progs.VS_TS_GS =
      m_progManager.createProgram(nvgl::ProgramManager::Definition(GL_VERTEX_SHADER, allDefines, "mvr_scene.vert.glsl"),
                                  nvgl::ProgramManager::Definition(GL_TESS_CONTROL_SHADER, allDefines, "mvr_scene.tcs.glsl"),
                                  nvgl::ProgramManager::Definition(GL_TESS_EVALUATION_SHADER, allDefines, "mvr_scene.tes.glsl"),
                                  nvgl::ProgramManager::Definition(GL_GEOMETRY_SHADER, allDefines, "mvr_scene.geo.glsl"),
                                  nvgl::ProgramManager::Definition(GL_FRAGMENT_SHADER, allDefines, "mvr_scene.frag.glsl"));
}

MVRPipeline::~MVRPipeline() {}

void MVRPipeline::setSettings(struct MVRSettings settings)
{
  m_settings = settings;

  PipelineVariants* progs;
  if(m_settings.m_renderMode == MVRSettings::RenderMode::SOFTWARE_FALLBACK)
  {
    progs = &m_programs.software;
  }
  else if(m_settings.m_renderMode == MVRSettings::RenderMode::SINGLE_PASS_STEREO)
  {
    progs = &m_programs.sps;
  }
  else if(m_settings.m_renderMode == MVRSettings::RenderMode::MULTI_VIEW_RENDERING)
  {
    if(m_settings.m_views == MVRSettings::Views::TWO_VIEWS)
    {
      progs = &m_programs.mvr;
    }
    else
    {
      progs = &m_programs.mvr_quad;
    }
  }


  if(m_settings.m_useTessellationShader && m_settings.m_useGeometryShader)
  {
    m_program = progs->VS_TS_GS;
  }
  else if(m_settings.m_useGeometryShader)
  {
    m_program = progs->VS_GS;
  }
  else if(m_settings.m_useTessellationShader)
  {
    m_program = progs->VS_TS;
  }
  else
  {
    m_program = progs->VS;
  }
}

void MVRPipeline::updateObjectUniforms()
{
  objectData.color = m_objectColor;

  Pipeline<vertexload::SceneDataMVR, vertexload::ObjectData>::updateObjectUniforms();
}

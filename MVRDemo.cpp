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

#include "MVRDemo.h"

typedef void(APIENTRYP PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)(GLenum  target,
                                                                GLenum  attachment,
                                                                GLuint  texture,
                                                                GLint   level,
                                                                GLint   baseViewIndex,
                                                                GLsizei numViews);
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR = nullptr;

bool MVRDemo::begin()
{
  if(!GLToriDemo::begin())
    return false;
  m_pipeline = std::make_unique<MVRPipeline>();

  m_torus.setVertexAttributeLocations(VERTEX_POS, VERTEX_NORMAL);

  nvgl::newFramebuffer(m_fbo);
  nvgl::newFramebuffer(m_blitFbo);

  glFramebufferTextureMultiviewOVR =
      (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)nvgl::ContextWindow::sysGetProcAddress("glFramebufferTextureMultiviewOVR");

  return glFramebufferTextureMultiviewOVR != nullptr;
}

void MVRDemo::initTextures(uint32_t width, uint32_t height, bool forceReInit)
{
  GLsizei oldWidth  = m_perViewWidth;
  GLsizei oldHeight = m_perViewHeight;

  // width & height are the window dimensions, the rendering dimensions
  // depend on whether 2 vs 4 views should be rendered:
  m_perViewWidth = width / 2;
  if(m_settings.m_views == MVRSettings::Views::QUAD_VIEW)
  {
    m_perViewHeight = height / 2;
  }
  else
  {
    m_perViewHeight = height;
  }

  if(!forceReInit)
  {
    // check if a re-init is not needed because the relevant settings didn't change
    if(oldWidth == m_perViewWidth && oldHeight == m_perViewHeight && m_settings.m_multisample == m_texturesAreMultisample)
    {
      return;
    }
  }

  nvgl::deleteTexture(m_colorTexArray);
  nvgl::deleteTexture(m_depthTexArray);

  GLsizei samples = 4;

  if(m_settings.m_multisample == true)
  {
    nvgl::newTexture(m_colorTexArray, GL_TEXTURE_2D_MULTISAMPLE_ARRAY);
    nvgl::newTexture(m_depthTexArray, GL_TEXTURE_2D_MULTISAMPLE_ARRAY);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_colorTexArray);
    glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, GL_RGBA8, m_perViewWidth, m_perViewHeight, MAX_VIEWS, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, m_depthTexArray);
    glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples, GL_DEPTH_COMPONENT24, m_perViewWidth,
                            m_perViewHeight, MAX_VIEWS, GL_FALSE);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0);
  }
  else
  {
    nvgl::newTexture(m_colorTexArray, GL_TEXTURE_2D_ARRAY);
    nvgl::newTexture(m_depthTexArray, GL_TEXTURE_2D_ARRAY);
    glTextureStorage3D(m_colorTexArray, 1, GL_RGBA8, m_perViewWidth, m_perViewHeight, MAX_VIEWS);
    glTextureStorage3D(m_depthTexArray, 1, GL_DEPTH_COMPONENT24, m_perViewWidth, m_perViewHeight, MAX_VIEWS);
  }
  m_texturesAreMultisample = m_settings.m_multisample;

  LOGOK("texture (re)init done\n");
}

void MVRDemo::end()
{
  nvgl::deleteTexture(m_colorTexArray);
  nvgl::deleteTexture(m_depthTexArray);
  nvgl::deleteFramebuffer(m_fbo);
  nvgl::deleteFramebuffer(m_blitFbo);
  GLToriDemo::end();
}

void MVRDemo::renderFrame(double time, uint32_t width, uint32_t height, GLuint fbo)
{
  static bool firstRun = true;

  validateSettings();
  initTextures(width, height, firstRun);
  firstRun = false;

  // Track the GPU frame time. This is only defined during multi-view rendering
  // if GL_EXT_multiview_timer_query is available.
  const bool timerQueryDefined =
      m_pipeline->supportMVR_timer_query || (m_settings.m_renderMode != MVRSettings::RenderMode::MULTI_VIEW_RENDERING);
  std::unique_ptr<nvgl::ProfilerGL::Section> gpuTimer;
  if(timerQueryDefined)
  {
    gpuTimer = std::make_unique<nvgl::ProfilerGL::Section>(m_profilerGL, "Whole Frame");
  }

  glViewport(0, 0, m_perViewWidth, m_perViewHeight);

  if(m_settings.m_multisample)
  {
    glEnable(GL_MULTISAMPLE);
  }
  else
  {
    glDisable(GL_MULTISAMPLE);
  }

  if(m_settings.m_useTessellationShader)
  {
    glPatchParameteri(GL_PATCH_VERTICES, 3);
  }

  updatePerFrameUniforms(m_perViewWidth, m_perViewHeight);
  m_pipeline->setSettings(m_settings);
  m_pipeline->setShaderProgram();
  m_pipeline->updateSceneUniforms();
  renderToTexture();
  blitToFramebuffer(fbo);
}

void MVRDemo::renderToTexture()
{
  size_t viewsThisFrame = 2;
  if(m_settings.m_views == MVRSettings::QUAD_VIEW)
  {
    viewsThisFrame = 4;
  }
  float     depth      = 1.0f;
  glm::vec4 background = glm::vec4(118.f / 255.f, 185.f / 255.f, 0.f / 255.f, 0.f / 255.f);

  // not using the extension here to present a fallback and performance baseline
  // here we fill the texture layers one by one, rendering two or four times
  //
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

  GLenum primitiveMode = GL_TRIANGLES;
  if(m_settings.m_useTessellationShader)
  {
    primitiveMode = GL_PATCHES;
  }

  if(m_settings.m_renderMode == MVRSettings::RenderMode::SOFTWARE_FALLBACK)
  {
    for(GLint i = 0; i < viewsThisFrame; ++i)
    {
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, i);
      glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexArray, 0, i);

      glClearBufferfv(GL_COLOR, 0, &background[0]);
      glClearBufferfv(GL_DEPTH, 0, &depth);

      glUniform1i(OFFSET_FALLBACK_ID, i);

      renderTori(m_numberOfTori, primitiveMode);
    }
  }
  else if(m_settings.m_renderMode == MVRSettings::RenderMode::SINGLE_PASS_STEREO)
  {

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexArray, 0);
    glClearBufferfv(GL_COLOR, 0, &background[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth);

    renderTori(m_numberOfTori, primitiveMode);
  }
  else if(m_settings.m_renderMode == MVRSettings::RenderMode::MULTI_VIEW_RENDERING)
  {
    glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 0, (GLsizei)viewsThisFrame);
    glFramebufferTextureMultiviewOVR(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexArray, 0, 0, (GLsizei)viewsThisFrame);
    glClearBufferfv(GL_COLOR, 0, &background[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth);

    renderTori(m_numberOfTori, primitiveMode);
  }
  else
  {
    assert(0);
  }
}

void MVRDemo::blitToFramebuffer(GLuint fbo)
{

  // blit the texture layers onto the screen:
  // differs only between a 2 view rendering vs a 4 view rendering
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_blitFbo);
  glNamedFramebufferTextureLayer(m_blitFbo, GL_DEPTH_ATTACHMENT, m_depthTexArray, 0, 0);

  if(m_settings.m_views == MVRSettings::Views::TWO_VIEWS)
  {
    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 0);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, 0, 0, m_perViewWidth, m_perViewHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 1);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, m_perViewWidth, 0, 2 * m_perViewWidth, m_perViewHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
  else
  {
    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 0);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, 0, 0, m_perViewWidth, m_perViewHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 1);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, m_perViewWidth, 0, 2 * m_perViewWidth, m_perViewHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 2);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, 0, m_perViewHeight, m_perViewWidth, 2 * m_perViewHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glNamedFramebufferTextureLayer(m_blitFbo, GL_COLOR_ATTACHMENT0, m_colorTexArray, 0, 3);
    glBlitFramebuffer(0, 0, m_perViewWidth, m_perViewHeight, m_perViewWidth, m_perViewHeight, 2 * m_perViewWidth,
                      2 * m_perViewHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  }
}

void MVRDemo::processUI(double time)
{
  GLToriDemo::processUI(time);

  int torusTessellationN = m_torus.getTessellationN();
  int torusTessellationM = m_torus.getTessellationM();

  ImGui::SetNextWindowPos(ImGuiH::dpiScaled(30, 120), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImGuiH::dpiScaled(455, 0), ImGuiCond_FirstUseEver);
  if(ImGui::Begin("MVR Demo Settings", nullptr))
  {
    ImGui::TextUnformatted("Input manually with CTRL+Click");

    ImGui::SliderInt("Tori", &m_numberOfTori, 1, 1000, "%d", ImGuiSliderFlags_None);
    ImGuiH::tooltip("Number of tori. Increase this number to make the CPU do more work.", false, 0.f);

    ImGui::SliderInt("Fragment load", &m_fragmentLoad, 1, 100, "%d", ImGuiSliderFlags_None);
    ImGuiH::tooltip(
        "Increase this number to make the fragment shader do more work. "
        "Specifically, this is the number of times the fragment shader computes 3D simplex noise.",
        false, 0.f);

    ImGui::SliderInt("Torus tessellation N", &torusTessellationN, 3, 64, "%d", ImGuiSliderFlags_None);
    ImGuiH::tooltip("Number of subdivisions around the axis of revolution.", false, 0.f);
    ImGui::SliderInt("Torus tessellation M", &torusTessellationM, 3, 64, "%d", ImGuiSliderFlags_None);
    ImGuiH::tooltip("Number of subdivisions of the ring.", false, 0.f);
    ImGui::Text("Triangle count per torus: %d", (int)m_torus.getTriangleCount());

    ImGui::Separator();
    if(m_settings.m_views == MVRSettings::Views::TWO_VIEWS)
    {
      ImGui::Text("Rendering 2 views");
      if(ImGui::Button("Switch to quad views"))
      {
        m_settings.m_views = MVRSettings::Views::QUAD_VIEW;
      }
    }
    else
    {
      ImGui::Text("Rendering 4 views");
      if(ImGui::Button("Switch to 2 views"))
      {
        m_settings.m_views = MVRSettings::Views::TWO_VIEWS;
      }
    }

    ImGui::Separator();
    if(m_settings.m_renderMode == MVRSettings::RenderMode::SOFTWARE_FALLBACK)
    {
      ImGui::Text("Render Mode: Software Fallback");
    }
    else if(m_settings.m_renderMode == MVRSettings::RenderMode::SINGLE_PASS_STEREO)
    {
      ImGui::Text("Render Mode: Single Pass Stereo");
    }
    else if(m_settings.m_renderMode == MVRSettings::RenderMode::MULTI_VIEW_RENDERING)
    {
      ImGui::Text("Render Mode: Multi-View Rendering");
    }

    if(ImGui::Button("Software Fallback"))
      m_settings.m_renderMode = MVRSettings::RenderMode::SOFTWARE_FALLBACK;
    if(ImGui::Button("Single Pass Stereo"))
      m_settings.m_renderMode = MVRSettings::RenderMode::SINGLE_PASS_STEREO;
    if(ImGui::Button("Multi-View Rendering"))
      m_settings.m_renderMode = MVRSettings::RenderMode::MULTI_VIEW_RENDERING;

    ImGui::Separator();
    ImGui::Checkbox("Multisample", &m_settings.m_multisample);
    ImGuiH::tooltip("Use 4x multisample anti-aliasing.", false, 0.f);
    ImGui::Checkbox("Use Geometry Shaders", &m_settings.m_useGeometryShader);
    ImGuiH::tooltip(
        "Render an arrow for the geometric normal of each triangle "
        "by adding mvr_scene.geo.glsl to the shading pipeline. This demonstrates "
        "that geometry shaders can be used with Single Pass Stereo, and if the "
        "GPU and driver support it, Multi-View Rendering as well.",
        false, 0.f);
    ImGui::Checkbox("Use Tessellation Shaders", &m_settings.m_useTessellationShader);
    ImGuiH::tooltip(
        "Subdivides each triangle using a tessellation factor of 4 "
        "and translates each vertex using 3D noise. This demonstrates that "
        "tessellation shaders can be used with Single Pass Stereo, and if the"
        "GPU and driver support it, Multi-View Rendering as well.",
        false, 0.f);

    ImGui::Separator();
    ImGui::Text("Extension support:");
    ImGui::Text("GL_NV_stereo_view_rendering: %s", (m_pipeline->supportSPS ? "yes" : "no"));
    ImGui::Text("GL_OVR_multiview & GL_OVR_multiview2: %s", (m_pipeline->supportMVR ? "yes" : "no"));
    ImGui::Text("GL_EXT_multiview_texture_multisample: %s", (m_pipeline->supportMVR_texture_multisample ? "yes" : "no"));
    ImGui::Text("GL_EXT_multiview_tessellation_geometry_shader: %s",
                (m_pipeline->supportMVR_tessellation_geometry_shader ? "yes" : "no"));
    ImGui::Text("GL_EXT_multiview_timer_query: %s", (m_pipeline->supportMVR_timer_query ? "yes" : "no"));

    ImGui::Separator();
  }
  ImGui::End();

  if(torusTessellationM != m_torus.getTessellationM() || torusTessellationN != m_torus.getTessellationN())
  {
    m_torus.setTessellation(torusTessellationN, torusTessellationM);
  }
}

void MVRDemo::updatePerFrameUniforms(uint32_t width, uint32_t height)
{
  auto view  = m_control.m_viewMatrix;
  auto iview = glm::inverse(view);

  auto proj = glm::perspective(45.f, float(width) / float(height), 0.01f, 10.0f);

  float     depth        = 1.0f;
  glm::vec4 background   = glm::vec4(118.f / 255.f, 185.f / 255.f, 0.f / 255.f, 0.f / 255.f);
  glm::vec3 eyePos_world = glm::vec3(iview[0][3], iview[1][3], iview[2][3]);
  glm::vec3 eyePos_view  = view * glm::vec4(eyePos_world, 1.0f);

  m_pipeline->sceneData.lightPos_world = glm::vec4(eyePos_world, 1.0f);

  m_pipeline->setProjectionMatrix(proj);
  m_pipeline->setViewMatrix(view);
  m_pipeline->sceneData.torusScale         = m_torus_scale;
  m_pipeline->sceneData.fragmentLoadFactor = m_fragmentLoad;

  if(m_settings.m_views == MVRSettings::Views::TWO_VIEWS)
  {
    float half_eye_distance             = 0.2f;
    m_pipeline->sceneData.projMatrix[0] = glm::translate(glm::mat4(1.0f), glm::vec3(-half_eye_distance, 0.0f, 0.0f)) * proj;
    m_pipeline->sceneData.projMatrix[1] = glm::translate(glm::mat4(1.0f), glm::vec3(+half_eye_distance, 0.0f, 0.0f)) * proj;

    //
    // The view matrix and eye position have to be identical for Single Pass Stereo
    // For Multi-View Rendering however, it could differ - as shown in the more complex
    // 4 views case.
    //
    for(size_t i = 0; i < 2; ++i)
    {
      m_pipeline->sceneData.viewMatrix[i] = view;
      m_pipeline->sceneData.viewProjMatrix[i] = m_pipeline->sceneData.projMatrix[i] * m_pipeline->sceneData.viewMatrix[i];
      m_pipeline->sceneData.eyepos_world[i] = glm::vec4(iview[0][3], iview[1][3], iview[2][3], 1.0f);
    }
  }
  else
  {
    // Quad view

    for(size_t i = 0; i < MAX_VIEWS; ++i)
    {
      //
      // All matrices can differ per view, but here we only change the view matrix:
      //
      m_pipeline->sceneData.projMatrix[i] = proj;
      m_pipeline->sceneData.viewMatrix[i] = glm::rotate(view, 90.0f * i, glm::vec3(1.0f, 0.0f, 0.0f));
      m_pipeline->sceneData.viewProjMatrix[i] = m_pipeline->sceneData.projMatrix[i] * m_pipeline->sceneData.viewMatrix[i];

      auto iview                            = glm::inverse(m_pipeline->sceneData.viewMatrix[i]);
      m_pipeline->sceneData.eyepos_world[i] = glm::vec4(iview[0][3], iview[1][3], iview[2][3], 1.0f);
    }
  }

  // upload to GPU:
  m_pipeline->updateSceneUniforms();
}

void MVRDemo::validateSettings()
{
  if(m_settings.m_views == MVRSettings::Views::QUAD_VIEW && m_settings.m_renderMode == MVRSettings::RenderMode::SINGLE_PASS_STEREO)
  {
    // Single Pass Stereo is limited to 2 views
    m_settings.m_views = MVRSettings::Views::TWO_VIEWS;
  }

  MVRPipeline* mvrPipeline = m_pipeline.get();
  assert(mvrPipeline);

  if(m_settings.m_renderMode == MVRSettings::RenderMode::SINGLE_PASS_STEREO && mvrPipeline->supportSPS == false)
  {
    m_settings.m_renderMode = MVRSettings::RenderMode::SOFTWARE_FALLBACK;
  }

  if(m_settings.m_renderMode == MVRSettings::RenderMode::MULTI_VIEW_RENDERING && mvrPipeline->supportMVR == false)
  {
    m_settings.m_renderMode = MVRSettings::RenderMode::SOFTWARE_FALLBACK;
  }

  if(m_settings.m_renderMode == MVRSettings::RenderMode::MULTI_VIEW_RENDERING
     && (m_settings.m_useGeometryShader || m_settings.m_useTessellationShader)
     && mvrPipeline->supportMVR_tessellation_geometry_shader == false)
  {
    m_settings.m_useGeometryShader     = false;
    m_settings.m_useTessellationShader = false;
  }
}

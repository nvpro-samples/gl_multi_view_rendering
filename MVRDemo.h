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

#include "GLToriDemo.h"

#include <glm/glm.hpp>
#include "common.h"
#include "MVRPipeline.h"
#include "MVRSettings.h"

#include <cstdint>
#include <memory>

/// @brief The main class showing the multi view rendering. The derived class
/// GLToriDemo<MVRPipeline> handles the mangement of the scene (tori).
/// First take a look at begin()/end() and renderFrame().
class MVRDemo : public GLToriDemo<MVRPipeline>
{
public:
  // called during init to setup pipeline and the geometry to render
  bool begin() override;

  // called to clean up
  void end() override;

  // called for each frame
  void renderFrame(double time, uint32_t width, uint32_t height, GLuint fbo) override;

private:
  void processUI(double time) override;
  void updatePerFrameUniforms(uint32_t width, uint32_t height);

  void renderToTexture();
  void blitToFramebuffer(GLuint fbo);

  // called at init and when the sample resizes
  void initTextures(uint32_t width, uint32_t height, bool forceReInit = false);

  // Framebuffer and textures to render into before the result
  // is blit into the sample frameworks FBO
  GLuint  m_fbo                    = 0;
  GLuint  m_blitFbo                = 0;
  GLuint  m_colorTexArray          = 0;
  GLuint  m_depthTexArray          = 0;
  GLsizei m_perViewHeight          = 0;
  GLsizei m_perViewWidth           = 0;
  bool    m_texturesAreMultisample = false;

  struct MVRSettings m_settings;

  // checks the settings and resolves unsupported combinations
  void validateSettings();
};

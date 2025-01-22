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

#include <cstdint>

struct MVRSettings
{
  bool m_multisample           = false;
  bool m_useGeometryShader     = false;
  bool m_useTessellationShader = false;
  enum Views
  {
    TWO_VIEWS,
    QUAD_VIEW
  } m_views = Views::TWO_VIEWS;
  enum RenderMode
  {
    SOFTWARE_FALLBACK,
    SINGLE_PASS_STEREO,
    MULTI_VIEW_RENDERING
  } m_renderMode = RenderMode::SOFTWARE_FALLBACK;
};

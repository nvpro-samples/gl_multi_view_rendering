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

#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "nvpsystem.hpp"
#include "stb_image.h"

#include "MVRDemo.h"

int const SAMPLE_SIZE_WIDTH  = 1200;
int const SAMPLE_SIZE_HEIGHT = 900;

// Default search path for shaders
std::vector<std::string> defaultSearchPaths{
    "./",
    "../",
    std::string(PROJECT_NAME),
    std::string("SPV_" PROJECT_NAME),
    NVPSystem::exePath() + PROJECT_RELDIRECTORY,
    NVPSystem::exePath() + std::string(PROJECT_RELDIRECTORY),
    NVPSystem::exePath() + std::string(PROJECT_RELDIRECTORY) + "shaders",
};

//--------------------------------------------------------------------------------------------------
//
//
int main(int argc, const char** argv)
{
  // setup some basic things for the sample, logging file for example
  NVPSystem system(PROJECT_NAME);

  MVRDemo sample;
  return sample.run(PROJECT_NAME, argc, argv, SAMPLE_SIZE_WIDTH, SAMPLE_SIZE_HEIGHT);
}

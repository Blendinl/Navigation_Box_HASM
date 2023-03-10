// Copyright 2017-2020 the Autoware Foundation, Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Co-developed by Tier IV, Inc. and Apex.AI, Inc.
//
// The changes made in this file, of which a summary is listed below, are copyrighted:
//
// Copyright (C) 2021 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.
//
// List of changes:
// * Changed namespace from autoware to aiim
// * Adding aiim prefix to includes
/// \file
/// \brief This file includes common type definition

#ifndef COMMON__TYPES_HPP_
#define COMMON__TYPES_HPP_

#include <cstdint>
#include <vector>

#include "aiim_autoware_common/visibility_control.hpp"

namespace aiim {
namespace common {
namespace types {
using bool8_t = bool;
using char8_t = char;
using uchar8_t = unsigned char;
using float32_t = float;
using float64_t = double;

/// pi = tau / 2
constexpr float32_t PI = 3.14159265359F;
/// pi/2
constexpr float32_t PI_2 = 1.5707963267948966F;
/// tau = 2 pi
constexpr float32_t TAU = 6.283185307179586476925286766559F;

struct COMMON_PUBLIC PointXYZIF {
    float32_t x{0};
    float32_t y{0};
    float32_t z{0};
    float32_t intensity{0};
    uint16_t id{0};
    static constexpr uint16_t END_OF_SCAN_ID = 65535u;
};

struct COMMON_PUBLIC PointXYZF {
    float32_t x{0};
    float32_t y{0};
    float32_t z{0};
    uint16_t id{0};
    static constexpr uint16_t END_OF_SCAN_ID = 65535u;
};

using PointBlock = std::vector<PointXYZIF>;
using PointPtrBlock = std::vector<const PointXYZIF*>;
/// \brief Stores basic configuration information, does some simple validity checking
static constexpr uint16_t POINT_BLOCK_CAPACITY = 512U;

// TODO(yunus.caliskan): switch to std::void_t when C++17 is available
/// \brief `std::void_t<> implementation
template <typename... Ts>
using void_t = void;
}  // namespace types
}  // namespace common
}  // namespace aiim

#endif  // COMMON__TYPES_HPP_

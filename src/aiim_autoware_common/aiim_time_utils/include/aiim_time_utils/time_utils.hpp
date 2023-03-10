// Copyright 2019 Christopher Ho
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// The changes made in this file, of which a summary is listed below, are copyrighted:
//
// Copyright (C) 2021 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.
//
// List of changes:
// * Added aiim prefix to time_utils path
// * Added aiim namespace
#ifndef TIME_UTILS__TIME_UTILS_HPP_
#define TIME_UTILS__TIME_UTILS_HPP_

#include <aiim_time_utils/visibility_control.hpp>
#include <builtin_interfaces/msg/duration.hpp>
#include <builtin_interfaces/msg/time.hpp>

#include <chrono>
namespace aiim {
namespace common {
namespace time_utils {
/// Convert from std::chrono::time_point to time message
TIME_UTILS_PUBLIC builtin_interfaces::msg::Time to_message(std::chrono::system_clock::time_point t);
/// Convert from std::chrono::duration to duration message
TIME_UTILS_PUBLIC builtin_interfaces::msg::Duration to_message(std::chrono::nanoseconds dt);
/// Convert from std::chrono::time_point from time message
TIME_UTILS_PUBLIC
std::chrono::system_clock::time_point from_message(builtin_interfaces::msg::Time t) noexcept;
/// Convert from std::chrono::duration from duration message
TIME_UTILS_PUBLIC
std::chrono::microseconds from_message(builtin_interfaces::msg::Duration dt) noexcept;
/// Standard interpolation
TIME_UTILS_PUBLIC std::chrono::nanoseconds interpolate(
        std::chrono::nanoseconds a, std::chrono::nanoseconds b, float t) noexcept;

namespace details {
template <typename TimeT>
TimeT duration_to_msg(std::chrono::nanoseconds dt);
}  // namespace details
}  // namespace time_utils
}  // namespace common
}  // namespace aiim

#endif  // TIME_UTILS__TIME_UTILS_HPP_

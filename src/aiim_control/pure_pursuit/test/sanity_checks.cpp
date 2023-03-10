// Copyright 2019 the Autoware Foundation
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

#include <aiim_autoware_common/types.hpp>
#include <aiim_motion_testing/motion_testing.hpp>
#include <aiim_time_utils/time_utils.hpp>
#include <gtest/gtest.h>

#include <algorithm>

#include "pure_pursuit/pure_pursuit.hpp"

using aiim::common::types::bool8_t;
using aiim::common::types::float32_t;
using aiim::control::pure_pursuit::Config;
using aiim::control::pure_pursuit::PurePursuit;
using aiim::control::pure_pursuit::VehicleControlCommand;
using aiim::motion::motion_testing::constant_velocity_trajectory;
using aiim::motion::motion_testing::make_state;
class sanity_checks : public ::testing::Test {
protected:
  float32_t steer_angle(const float32_t dx, const float32_t lookahead) const {
    return std::atan(2.0F * dx * cfg_.get_distance_front_rear_wheel() /
                     (lookahead * lookahead));
  }
  void check_steer(const VehicleControlCommand cmd, const float32_t guess,
                   const float32_t TOL = 1.0E-4F) {
    EXPECT_LT(std::fabs(cmd.front_wheel_angle_rad - guess), TOL)
        << cmd.front_wheel_angle_rad << ", " << guess;
    EXPECT_LT(std::fabs(cmd.rear_wheel_angle_rad), TOL);
    if (HasFailure()) {
      ADD_FAILURE();
    }
  }
  // 200 ms lookahead horizon, 2m wheelbase length
  Config cfg_{0.5F, 100.0F, 0.2F, true, false, 2.0F, 0.1F, 2.0F};
  PurePursuit controller_{cfg_};
}; // sanity checks

constexpr auto ms100 = std::chrono::milliseconds{100LL};

struct StraightTestParam {
  float32_t dx;
  float32_t dy;
  bool8_t is_pointing_north;
};

class sanity_checks_axis_aligned_straight
    : public sanity_checks,
      public ::testing::WithParamInterface<StraightTestParam> {};

// Basic straight test case with easy to compute lookaheads, no longitudinal
// control Double slashes on the right to avoid -Wcomment warnings
//                                         //
//      >--------------                    //
//                      \                  //
//      -----------------T---------->      //
TEST_P(sanity_checks_axis_aligned_straight, basic) {
  const auto x0 = 0.0F;
  const auto y0 = 0.0F;
  const auto th0 = 0.0F;
  const auto dx = GetParam().dx;
  const auto dy = GetParam().dy;
  const auto L = std::max(std::sqrt((dx * dx) + (dy * dy)),
                          cfg_.get_minimum_lookahead_distance());
  const auto v0 = L / cfg_.get_speed_to_lookahead_ratio();
  const auto now = std::chrono::system_clock::now();
  const auto traj = constant_velocity_trajectory(x0, y0, th0, v0, ms100);
  const auto s = make_state(x0, y0 + dy, th0, v0, 0.0F, 0.0F, now);
  controller_.set_trajectory(traj);
  const auto cmd = controller_.compute_command(s);
  constexpr auto TOL = 1.0E-4F;
  EXPECT_LT(std::fabs(cmd.long_accel_mps2), TOL);
  const auto guess = steer_angle(-dy, L);
  check_steer(cmd, guess, TOL);
}

INSTANTIATE_TEST_CASE_P(basic, sanity_checks_axis_aligned_straight,
                        ::testing::Values(StraightTestParam{0.0F, 0.0F, false},
                                          StraightTestParam{3.0F, 4.0F, false},
                                          StraightTestParam{-5.0F, 12.0F, false}
                                          // cppcheck-suppress syntaxError
                                          ), );

// oriented_straight: exercise some basic transform stuff
//      ^
//      |
//   __ T
//  /   |
//  |   |
//  ^
class sanity_checks_oriented_straight
    : public sanity_checks,
      public ::testing::WithParamInterface<StraightTestParam> {};

TEST_P(sanity_checks_oriented_straight, basic) {
  const auto x0 = 0.0F;
  const auto y0 = 0.0F;
  const auto th0 = (GetParam().is_pointing_north ? 3.14159F : -3.14159F) / 2.0F;
  const auto dx = GetParam().dx;
  const auto dy = GetParam().dy;
  const auto L = std::max(std::sqrt((dx * dx) + (dy * dy)),
                          cfg_.get_minimum_lookahead_distance());
  const auto v0 = L / cfg_.get_speed_to_lookahead_ratio();
  const auto now = std::chrono::system_clock::now();
  const auto traj = constant_velocity_trajectory(x0, y0, th0, v0, ms100);
  const auto s = make_state(x0 + dx, y0, th0, v0, 0.0F, 0.0F, now);
  controller_.set_trajectory(traj);
  const auto cmd = controller_.compute_command(s);
  constexpr auto TOL = 1.0E-4F;
  EXPECT_LT(std::fabs(cmd.long_accel_mps2), TOL);
  const auto dx_ = GetParam().is_pointing_north ? dx : -dx;
  const auto guess = steer_angle(dx_, L);
  check_steer(cmd, guess, TOL);
}

INSTANTIATE_TEST_CASE_P(
    basic, sanity_checks_oriented_straight,
    ::testing::Values(StraightTestParam{0.0F, 0.0F, false},
                      StraightTestParam{3.0F, 4.0F, false},
                      StraightTestParam{-5.0F, 12.0F, false},
                      StraightTestParam{0.0F, 0.0F, true},
                      StraightTestParam{3.0F, 4.0F, true},
                      StraightTestParam{-5.0F, 12.0F, true}), );

// Orthogonal orientation: easy to compute offset for
// Double slashes on the right to avoid -Wcomment warnings
//      __             //
//     /  \            //
//    |   |            //
//    ^---T------->    //
class sanity_checks_orthogonal_orientation
    : public sanity_checks,
      public ::testing::WithParamInterface<StraightTestParam> {};

TEST_P(sanity_checks_orthogonal_orientation, basic) {
  const auto x0 = 0.0F;
  const auto y0 = 0.0F;
  const auto th0 = 0.0F;
  const auto dy = GetParam().dy;
  const auto L = std::max(std::fabs(dy), cfg_.get_minimum_lookahead_distance());
  const auto v0 = L / cfg_.get_speed_to_lookahead_ratio();
  const auto now = std::chrono::system_clock::now();
  const auto traj = constant_velocity_trajectory(x0, y0, th0, v0, ms100);
  const auto th = (GetParam().is_pointing_north ? 3.14159F : -3.14159F) / 2.0F;
  const auto s = make_state(x0, y0, th, v0, 0.0F, 0.0F, now);
  controller_.set_trajectory(traj);
  const auto cmd = controller_.compute_command(s);
  constexpr auto TOL = 1.0E-4F;
  EXPECT_LT(std::fabs(cmd.long_accel_mps2), TOL);
  const auto dx = GetParam().is_pointing_north ? -L : L;
  const auto guess = steer_angle(dx, L);
  check_steer(cmd, guess, TOL);
}

INSTANTIATE_TEST_CASE_P(basic, sanity_checks_orthogonal_orientation,
                        ::testing::Values(StraightTestParam{0.0F, 5.0F, false},
                                          StraightTestParam{0.0F, 5.0F,
                                                            true}), );

// A track which curves like (ellipsoid):
//
// o        |
//        __/
//  ___---
class sanity_checks_other : public sanity_checks {
protected:
  static constexpr auto size{100U};
  static constexpr auto th_per_iter{3.14159F /
                                    static_cast<float32_t>(2U * size)};
  static constexpr auto r0{1.0F};
  static constexpr auto r_rate{0.1F};
  using Trajectory = aiim::control::pure_pursuit::Trajectory;
  Trajectory curved_track() {
    Trajectory ret{};
    ret.points.reserve(size);
    for (auto idx = 0U; idx < size; ++idx) {
      aiim_autoware_msgs::msg::TrajectoryPoint pt{};
      const auto fdx = static_cast<float32_t>(idx);
      const auto r = r0 + (r_rate * fdx);
      const auto th = th_per_iter * fdx;
      pt.x = r * std::cos(th);
      pt.y = r * std::sin(th);
      pt.heading =
          aiim::motion::motion_common::from_angle(th + (3.14159F / 2.0F));
      pt.longitudinal_velocity_mps = 10.0F;
      pt.time_from_start = aiim::common::time_utils::to_message(
          std::chrono::milliseconds{100LL} * idx);
      ret.points.push_back(pt);
    }
    return ret;
  }
};

// curved track TODO(c.ho) parameterize this test
TEST_F(sanity_checks_other, curved_track_towards) {
  constexpr auto v0 = 10.0F;
  const auto L = cfg_.get_speed_to_lookahead_ratio() * v0;
  const auto now = std::chrono::system_clock::now();
  const auto state =
      make_state(0.0F, 0.0F, 3.14159F / 2.0F, v0, 0.0F, 0.0F, now);
  auto traj = curved_track();
  traj.header = state.header;
  controller_.set_trajectory(traj);
  const auto cmd = controller_.compute_command(state);
  constexpr auto TOL = 1.0E-4F;
  EXPECT_LT(std::fabs(cmd.long_accel_mps2), TOL);
  // Compute theta from L
  const auto th = (L - r0) * th_per_iter / r_rate;
  const auto dx = -std::cos(th) * L;
  const auto guess = steer_angle(dx, L);
  check_steer(cmd, guess, TOL);
}

// longitudinal control TODO(c.ho) parameterize this test
TEST_F(sanity_checks, longitudinal_control) {
  const auto x0 = 0.0F;
  const auto y0 = 0.0F;
  const auto th0 = 0.0F;
  const auto v0 = 1.0F;
  const auto v = 10.0F;
  const auto L = std::max(v0 * cfg_.get_speed_to_lookahead_ratio(),
                          cfg_.get_minimum_lookahead_distance());
  const auto now = std::chrono::system_clock::now();
  const auto traj = constant_velocity_trajectory(x0, y0, th0, v, ms100);
  const auto s = make_state(x0, y0, th0, v0, 0.0F, 0.0F, now);
  controller_.set_trajectory(traj);
  const auto cmd = controller_.compute_command(s);
  constexpr auto TOL = 1.0E-4F;
  // v^2 = 2ad
  const auto guess = ((v * v) - (v0 * v0)) / (2.0F * L);
  EXPECT_LT(std::fabs(cmd.long_accel_mps2 - guess), TOL);
  EXPECT_LT(std::fabs(cmd.front_wheel_angle_rad), TOL);
  EXPECT_LT(std::fabs(cmd.rear_wheel_angle_rad), TOL);
}

// TODO(c.ho) acceleration-based lookahead distance
// TODO(c.ho) reverse test cases

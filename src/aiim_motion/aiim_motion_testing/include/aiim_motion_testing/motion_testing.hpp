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
// The changes made in this file, of which a summary is listed below, are
// copyrighted:
//
// Copyright (C) 2021 by NavInfo Europe B.V. The Netherlands - All rights
// reserved Information classification: Confidential This content is protected
// by international copyright laws. Reproduction and distribution is prohibited
// without written permission.
//
// List of changes:
// * Adding aiim namespace
// * Adding aiim prefix
#ifndef aiim_motion_testing__aiim_motion_testing_HPP_
#define aiim_motion_testing__aiim_motion_testing_HPP_

#include <aiim_autoware_msgs/msg/trajectory.hpp>
#include <aiim_autoware_msgs/msg/vehicle_control_command.hpp>
#include <aiim_autoware_msgs/msg/vehicle_kinematic_state.hpp>
#include <aiim_motion_testing/visibility_control.hpp>

#include <chrono>
#include <random>
namespace aiim {
namespace motion {
namespace motion_testing {
using Generator = std::mt19937;
using State = aiim_autoware_msgs::msg::VehicleKinematicState;
using Point = aiim_autoware_msgs::msg::TrajectoryPoint;
using Trajectory = aiim_autoware_msgs::msg::Trajectory;
// TODO(c.ho) Make these more modular

/// \brief Makes a state, intended to make message generation more terse
aiim_motion_testing_PUBLIC State
make_state(float x0, float y0, float heading, float v0, float a0,
           float turn_rate, std::chrono::system_clock::time_point t);

/// \brief Generates a state from a normal distribution with the following
/// bounds:
///       TODO(c.ho)
aiim_motion_testing_PUBLIC State generate_state(Generator &gen);

/// \brief Generates a trajectory assuming the starting state, a bicycle model,
/// and additive noise applied to XXX Note: not implemented
aiim_motion_testing_PUBLIC Trajectory
generate_trajectory(const State &start_state, Generator &gen);

/// \brief Generate a trajectory given the start state, assuming the highest
/// derivatives are held
///        constant.
/// Note: heading_rate behavior will be kind of off TODO(c.ho)
/// Note: lateral_velocity_mps will not be respected TODO(cho)
aiim_motion_testing_PUBLIC Trajectory
constant_trajectory(const State &start_state, std::chrono::nanoseconds dt);
/// \brief Generates a constant velocity trajectory

aiim_motion_testing_PUBLIC Trajectory
bad_heading_trajectory(const State &start_state, std::chrono::nanoseconds dt);
/// \brief Generates a constant velocity trajectory with invalid heading values

aiim_motion_testing_PUBLIC Trajectory constant_velocity_trajectory(
    float x0, float y0, float heading, float v0, std::chrono::nanoseconds dt);
/// \brief Generates a constant acceleration trajectory
aiim_motion_testing_PUBLIC Trajectory
constant_acceleration_trajectory(float x0, float y0, float heading, float v0,
                                 float a0, std::chrono::nanoseconds dt);
/// \brief Generates a constant velocity and constant turn rate trajectory
aiim_motion_testing_PUBLIC Trajectory constant_velocity_turn_rate_trajectory(
    float x0, float y0, float heading, float v0, float turn_rate,
    std::chrono::nanoseconds dt);
/// \brief Generates a constant acceleration and constant turn rate trajectory
aiim_motion_testing_PUBLIC Trajectory
constant_acceleration_turn_rate_trajectory(float x0, float y0, float heading,
                                           float v0, float a0, float turn_rate,
                                           std::chrono::nanoseconds dt);

/// Given a trajectory, advance state to next trajectory point, with normally
/// distributed noise Note: This version takes "hint" as gospel, and doesn't try
/// to do any time/space matching Note: not implemented
aiim_motion_testing_PUBLIC void
next_state(const Trajectory &trajectory, State &state, uint32_t hint,
           Generator *gen = nullptr); // TODO(c.ho) std::optional NOLINT
// TODO(c.ho) version that takes control commands

using Index = decltype(Trajectory::points)::size_type;
using Real = decltype(Point::x);
/// Checks that a trajectory makes constant progress towards a target; returns
/// first index of point that doesn't advance towards target, otherwise size of
/// trajectory heading tolerance is in dot product space of 2d quaternion
aiim_motion_testing_PUBLIC Index
progresses_towards_target(const Trajectory &trajectory, const Point &target,
                          Real heading_tolerance = Real{0.006F});

/// Checks that a trajectory is more or less dynamically feasible given the
/// derivatives; tolerance is relative tolerance of trajectory, index is first
/// point that is not dynamically feasible, trajectory.size() if completely
/// feasible
aiim_motion_testing_PUBLIC Index
dynamically_feasible(const Trajectory &trajectory, Real tolerance = 0.05F);
} // namespace motion_testing
} // namespace motion
} // namespace aiim

#endif // aiim_motion_testing__aiim_motion_testing_HPP_

// Copyright 2020 Embotech AG, Zurich, Switzerland
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
// * Using aiim namespace
// * Adding aiim prefix
#include <gtest/gtest.h>
#include <aiim_autoware_msgs/msg/trajectory.hpp>
#include <aiim_motion_common/config.hpp>
#include <aiim_motion_testing/motion_testing.hpp>
#include <rclcpp/rclcpp.hpp>
#include <recordreplay_planner_nodes/recordreplay_planner_node.hpp>

#include <algorithm>
#include <chrono>
#include <memory>

using aiim::motion::motion_testing::make_state;
using aiim::planning::recordreplay_planner_nodes::RecordReplayPlannerNode;
using aiim_autoware_msgs::msg::Trajectory;
using std::chrono::system_clock;
using State = aiim_autoware_msgs::msg::VehicleKinematicState;

using aiim::motion::motion_common::VehicleConfig;

TEST(mytest_base, basic) {
    const auto heading_weight = 0.1;
    const auto min_record_distance = 0.0;
    rclcpp::init(0, nullptr);

    rclcpp::NodeOptions node_options_rr, node_options_ob;
    node_options_ob.append_parameter_override("vehicle.cg_to_front_m", 1.0);
    node_options_ob.append_parameter_override("vehicle.cg_to_rear_m", 1.0);
    node_options_ob.append_parameter_override("vehicle.front_corner_stiffness", 0.5);
    node_options_ob.append_parameter_override("vehicle.rear_corner_stiffness", 0.5);
    node_options_ob.append_parameter_override("vehicle.mass_kg", 1500.0);
    node_options_ob.append_parameter_override("vehicle.yaw_inertia_kgm2", 12.0);
    node_options_ob.append_parameter_override("vehicle.width_m", 2.0);
    node_options_ob.append_parameter_override("vehicle.front_overhang_m", 0.5);
    node_options_ob.append_parameter_override("vehicle.rear_overhang_m", 0.2);
    node_options_ob.append_parameter_override("safety_factor", 2.0);
    node_options_ob.append_parameter_override("stop_margin", 5.0);
    node_options_ob.append_parameter_override("trajectory_smoother.kernel_std", 5.0);
    node_options_ob.append_parameter_override("trajectory_smoother.kernel_size", 25);
    node_options_ob.append_parameter_override("staleness_threshold_ms", 500);
    node_options_ob.append_parameter_override("target_frame_id", "map");

    node_options_rr.append_parameter_override("heading_weight", heading_weight);
    node_options_rr.append_parameter_override("min_record_distance", min_record_distance);
    node_options_rr.append_parameter_override("enable_object_collision_estimator", true);
    node_options_rr.append_parameter_override("enable_object_collision_estimator", true);
    node_options_rr.append_parameter_override("goal_distance_threshold_m", 0.75);
    node_options_rr.append_parameter_override("goal_angle_threshold_rad", aiim::common::types::PI_2);
    auto plannernode = std::make_shared<RecordReplayPlannerNode>(node_options_rr);

    using PubAllocT = rclcpp::PublisherOptionsWithAllocator<std::allocator<void>>;
    const auto publisher = std::make_shared<rclcpp::Node>("recordreplay_node_testpublisher");
    const auto pub =
            publisher->create_publisher<State>("vehicle_state", rclcpp::QoS{10}.transient_local(), PubAllocT{});

    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(plannernode);
    exec.add_node(publisher);

    auto dummy_state = std::make_shared<State>();
    pub->publish(*dummy_state);
    EXPECT_NO_THROW(exec.spin_some(std::chrono::milliseconds(100LL)));

    // TODO(s.me) actually do what I planned on doing in the launch_testing file here.
    // This is tracked by https://gitlab.com/autowarefoundation/autoware.auto/AutowareAuto/issues/273.

    rclcpp::shutdown();
}

// Copyright 2020 Arm Limited
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
// * Adding aiim prefix to includes
// * Changed dependencies

#include <gtest/gtest.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <aiim_motion_testing/motion_testing.hpp>
#include <aiim_tf2/tf2_aiim_autoware_msgs.hpp>
#include <chrono>
#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <thread>

#include "object_collision_estimator_nodes/object_collision_estimator_node.hpp"

using aiim::common::types::float32_t;
using aiim::motion::motion_common::VehicleConfig;
using aiim::motion::motion_testing::constant_velocity_trajectory;
using aiim::planning::object_collision_estimator_nodes::ObjectCollisionEstimatorNode;
using aiim_autoware_msgs::msg::BoundingBox;
using aiim_autoware_msgs::msg::BoundingBoxArray;
using aiim_autoware_msgs::msg::Trajectory;
using std::chrono::system_clock;

using namespace std::chrono_literals;

const auto make_point(const float32_t x, const float32_t y) {
    geometry_msgs::msg::Point32 p;
    p.x = x;
    p.y = y;
    return p;
}

void object_collision_estimator_node_test(std::size_t trajectory_length, std::size_t obstacle_bbox_idx) {
    rclcpp::init(0, nullptr);

    // define hard coded dummy node parameters
    rclcpp::NodeOptions node_options;
    node_options.append_parameter_override("vehicle.cg_to_front_m", 1.0);
    node_options.append_parameter_override("vehicle.cg_to_rear_m", 1.0);
    node_options.append_parameter_override("vehicle.front_corner_stiffness", 0.5);
    node_options.append_parameter_override("vehicle.rear_corner_stiffness", 0.5);
    node_options.append_parameter_override("vehicle.mass_kg", 1000.0);
    node_options.append_parameter_override("vehicle.yaw_inertia_kgm2", 12.0);
    node_options.append_parameter_override("vehicle.width_m", 2.0);
    node_options.append_parameter_override("vehicle.front_overhang_m", 0.5);
    node_options.append_parameter_override("vehicle.rear_overhang_m", 0.5);
    node_options.append_parameter_override("safety_factor", 1.1);
    node_options.append_parameter_override("staleness_threshold_ms", 500);
    node_options.append_parameter_override("target_frame_id", "map");
    node_options.append_parameter_override("trajectory_smoother.kernel_std", 5.0F);
    node_options.append_parameter_override("trajectory_smoother.kernel_size", 25);
    node_options.append_parameter_override("stop_margin", 0.0);
    // create collision estimator node
    auto estimator_node = std::make_shared<ObjectCollisionEstimatorNode>(node_options);

    // create a publisher node to publish dummy obstacles
    using PubAllocT = rclcpp::PublisherOptionsWithAllocator<std::allocator<void>>;
    const auto dummy_obstacle_publisher =
            std::make_shared<rclcpp::Node>("object_collision_estimator_node_test_publisher");
    const auto pub = dummy_obstacle_publisher->create_publisher<BoundingBoxArray>(
            "obstacle_topic", rclcpp::QoS{10}.transient_local(), PubAllocT{});

    // create a client node to call the service interface of the collision estimator node
    std::shared_ptr<rclcpp::Node> estimate_collision_client_node =
            rclcpp::Node::make_shared("estimate_collision_client");
    rclcpp::Client<aiim_autoware_msgs::srv::ModifyTrajectory>::SharedPtr estimate_collision_client =
            estimate_collision_client_node->create_client<aiim_autoware_msgs::srv::ModifyTrajectory>(
                    "estimate_collision");

    // create a executor and add all nodes to it
    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(estimator_node);
    exec.add_node(dummy_obstacle_publisher);
    exec.add_node(estimate_collision_client_node);

    // produce fake trajectory and obstacles
    const std::chrono::milliseconds dt(100);
    auto trajectory =
            constant_velocity_trajectory(0, 0, 1, 10, std::chrono::duration_cast<std::chrono::nanoseconds>(dt));
    trajectory.points.resize(trajectory_length);

    // insert an obstacle that blocks the trajectory
    BoundingBoxArray bbox_array{};

    if (obstacle_bbox_idx < trajectory_length) {
        BoundingBox obstacle_bbox{};

        auto obstacle_point = trajectory.points[obstacle_bbox_idx];
        obstacle_bbox.centroid = make_point(obstacle_point.x, obstacle_point.y);
        obstacle_bbox.size = make_point(0.5, 0.5);
        obstacle_bbox.orientation.x = 1.0F;
        obstacle_bbox.corners = {
                make_point(obstacle_point.x - obstacle_bbox.size.x / 2, obstacle_point.y - obstacle_bbox.size.y / 2),
                make_point(obstacle_point.x + obstacle_bbox.size.x / 2, obstacle_point.y - obstacle_bbox.size.y / 2),
                make_point(obstacle_point.x + obstacle_bbox.size.x / 2, obstacle_point.y + obstacle_bbox.size.y / 2),
                make_point(obstacle_point.x - obstacle_bbox.size.x / 2, obstacle_point.y + obstacle_bbox.size.y / 2)};

        bbox_array.boxes.push_back(obstacle_bbox);
    }
    bbox_array.header.frame_id = "map";
    bbox_array.header.stamp = rclcpp::Clock().now();

    // publish the list of obstacles to the obstacles topic
    pub->publish(bbox_array);

    exec.spin_some();

    // wait till the service interface is ready
    while (!estimate_collision_client->wait_for_service(1s)) {
        EXPECT_EQ(rclcpp::ok(), true);
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "service not available, waiting again...");
    }

    // create a request message that can be sent to the service interface
    auto request = std::make_shared<aiim_autoware_msgs::srv::ModifyTrajectory::Request>();
    request->original_trajectory = trajectory;

    // sent the request to the service interface
    auto result_future = estimate_collision_client->async_send_request(request);

    // Wait for the result to be returned
    std::chrono::milliseconds span(100);
    while (result_future.wait_for(span) == std::future_status::timeout) {
        exec.spin_some();
    }

    auto result = result_future.get();
    auto updated_trajectory = result->modified_trajectory;

    // check the results are what expected
    if (obstacle_bbox_idx < trajectory_length) {
        if (obstacle_bbox_idx != 0) {
            // Check that the trajectory has been curtailed
            EXPECT_EQ(updated_trajectory.points.size(), obstacle_bbox_idx - 1);
        } else {
            // Check that the trajectory has been curtailed
            EXPECT_EQ(updated_trajectory.points.size(), 0U);
        }

        // Check that the last point has zero velocity and acceleration
        if (updated_trajectory.points.size() != 0) {
            EXPECT_EQ(updated_trajectory.points[updated_trajectory.points.size() - 1].longitudinal_velocity_mps, 0);
            EXPECT_EQ(updated_trajectory.points[updated_trajectory.points.size() - 1].acceleration_mps2, 0);
        }
    } else {
        // no obstacle
        EXPECT_EQ(updated_trajectory.points.size(), trajectory_length);

        if (updated_trajectory.points.size() != 0) {
            EXPECT_NE(updated_trajectory.points[updated_trajectory.points.size() - 1].longitudinal_velocity_mps, 0);
        }
    }

    // clean up
    rclcpp::shutdown();
}

TEST(object_collision_estimator_node, sanity) { object_collision_estimator_node_test(100, 40); }

TEST(object_collision_estimator_node, short_trajectory) {
    object_collision_estimator_node_test(3, 1);
    object_collision_estimator_node_test(3, 2);

    // no obstacles
    object_collision_estimator_node_test(2, 2);
    object_collision_estimator_node_test(1, 2);
    object_collision_estimator_node_test(0, 2);
}

TEST(object_collision_estimator_node, emergency_stop) {
    object_collision_estimator_node_test(100, 0);
    object_collision_estimator_node_test(100, 1);
}

TEST(object_collision_estimator_node, no_obstacle) { object_collision_estimator_node_test(100, 101); }

// Copyright 2020 Christopher Ho
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
// * Added aiim prefix
// * Added aiim namespace
#include <aiim_motion_testing/motion_testing.hpp>
#include <aiim_motion_testing_nodes/motion_testing_publisher.hpp>
#include <aiim_motion_testing_nodes/wait_for_matched.hpp>
#include <aiim_time_utils/time_utils.hpp>
#include <controller_common_nodes/controller_base_node.hpp>
#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using aiim::control::controller_common::BehaviorConfig;
using aiim::control::controller_common::ControllerBase;
using aiim::control::controller_common::ControlReference;
using aiim::control::controller_common_nodes::Command;
using aiim::control::controller_common_nodes::ControllerBaseNode;
using aiim::control::controller_common_nodes::ControllerPtr;
using aiim::motion::motion_testing::make_state;
using aiim::motion::motion_testing_nodes::State;
using aiim::motion::motion_testing_nodes::TFMessage;
using aiim::motion::motion_testing_nodes::Trajectory;
using geometry_msgs::msg::TransformStamped;

class sanity_check : public ::testing::Test {
protected:
  void SetUp() { rclcpp::init(0, nullptr); }
  void TearDown() { (void)rclcpp::shutdown(); }
}; // sanity_check

constexpr auto cmd_topic = "test_controller_base_node_cmd_tf";
constexpr auto state_topic = "test_controller_base_node_state_tf";
constexpr auto tf_topic = "test_controller_base_node_tf_tf";
constexpr auto traj_topic = "test_controller_base_node_traj_tf";
constexpr auto static_tf_topic = "test_controller_base_node_static_tf_tf";

// A simple controller instance to pass-through the transformed stuff
class TestTFController : public ControllerBase {
public:
  TestTFController()
      : ControllerBase{BehaviorConfig{3.0F, std::chrono::milliseconds(100L),
                                      ControlReference::SPATIAL}} {}

  const std::vector<State> &states() const noexcept { return m_states; }

protected:
  Command compute_command_impl(const State &state) override {
    Command ret{};
    m_states.push_back(state);
    return ret;
  }

private:
  std::vector<State> m_states{};
}; // class TestTFController

class TestTFControllerNode : public ControllerBaseNode {
public:
  TestTFControllerNode()
      : ControllerBaseNode{"test_controller_base_node_tf",
                           "",
                           cmd_topic,
                           state_topic,
                           tf_topic,
                           traj_topic,
                           "",
                           static_tf_topic} {
    auto ptr = std::make_unique<TestTFController>();
    m_controller = ptr.get();
    ControllerBaseNode::set_controller(std::move(ptr));
  }

  const TestTFController &controller() const noexcept { return *m_controller; }

private:
  TestTFController *m_controller{};
}; // class TestControllerNode

class transforms : public ::testing::Test {
protected:
  void SetUp() { rclcpp::init(0, nullptr); }
  void TearDown() { (void)rclcpp::shutdown(); }
}; // sanity_check

// Publish static tf: nav_base -> base_link
// Publish tfs: base_link -> odom
// Publish trajectory: odom
// Publish state: nav_base
// Expect result state in odom
// Note: testing exotic transforms is outside the scope of this test; only
// testing that transforms are applied
TEST_F(transforms, static_dynamic_tfs) {
  // Publisher Node
  const auto pub_node = std::make_shared<rclcpp::Node>("test_tf_pub_node");
  const auto traj_pub =
      pub_node->create_publisher<Trajectory>(traj_topic, rclcpp::QoS{10LL});
  const auto state_pub =
      pub_node->create_publisher<State>(state_topic, rclcpp::QoS{10LL});
  const auto tf_pub =
      pub_node->create_publisher<TFMessage>(tf_topic, rclcpp::QoS{10LL});
  const rclcpp::QoS transient_local_qos = rclcpp::QoS{10LL}.transient_local();
  const auto static_tf_pub = pub_node->create_publisher<TFMessage>(
      static_tf_topic, transient_local_qos);
  // Controller node
  const auto ctrl = std::make_shared<TestTFControllerNode>();
  // Match
  (void)aiim::motion::motion_testing_nodes::wait_for_matched(*traj_pub, 1U);
  (void)aiim::motion::motion_testing_nodes::wait_for_matched(*state_pub, 1U);
  (void)aiim::motion::motion_testing_nodes::wait_for_matched(*tf_pub, 1U);
  (void)aiim::motion::motion_testing_nodes::wait_for_matched(*static_tf_pub,
                                                             1U);
  // Spin
  {
    const auto make_tf = []() -> auto {
      TFMessage tf_msg{};
      geometry_msgs::msg::TransformStamped tf{};
      tf.transform.translation.x = 1.0;
      tf.transform.translation.y = 2.0;
      tf.transform.rotation.w = 1.0;
      tf_msg.transforms.emplace_back(std::move(tf));
      return tf_msg;
    };
    using aiim::motion::motion_testing::constant_velocity_trajectory;
    // State
    const auto stamp = std::chrono::system_clock::time_point{} +
                       std::chrono::milliseconds{50LL};
    auto state = make_state(1.0F, -2.0F, 0.0F, 3.0F, 0.0F, 0.0F, stamp);
    state.header.frame_id = "nav_base";
    // Traj
    auto traj = constant_velocity_trajectory(0.0, 0.0, 0.0F, 1.0F,
                                             std::chrono::milliseconds{100LL});
    traj.header.frame_id = "odom";
    traj.header.stamp = state.header.stamp;
    traj.header.stamp.nanosec -= 50000;
    traj_pub->publish(traj);
    rclcpp::executors::SingleThreadedExecutor exec{};
    exec.add_node(ctrl);
    while (ctrl->controller().states().empty()) {
      // Publish more:
      traj_pub->publish(traj);
      state_pub->publish(state);
      // TF
      {
        auto tf_msg = make_tf();
        // Add tf after
        tf_msg.transforms.front().header.stamp = state.header.stamp;
        tf_msg.transforms.front().header.stamp.nanosec += 1000;
        tf_msg.transforms.front().header.frame_id = "base_link";
        tf_msg.transforms.front().child_frame_id = "odom";
        // Add tf before
        tf_msg.transforms.push_back(tf_msg.transforms.front());
        tf_msg.transforms.back().header.stamp.nanosec -= 2000;
        tf_pub->publish(tf_msg);
      }
      // Static tf
      {
        // Add tf after
        auto static_tf_msg = make_tf();
        static_tf_msg.transforms.front().header.stamp = state.header.stamp;
        static_tf_msg.transforms.front().header.stamp.nanosec += 1000;
        static_tf_msg.transforms.front().header.frame_id = "nav_base";
        static_tf_msg.transforms.front().child_frame_id = "base_link";
        // Add tf before
        static_tf_msg.transforms.push_back(static_tf_msg.transforms.front());
        static_tf_msg.transforms.back().header.stamp.nanosec -= 2000;
        static_tf_pub->publish(static_tf_msg);
      }
      // Spin
      exec.spin_some();
    }
  }
  // Check results
  {
    for (const auto &state : ctrl->controller().states()) {
      EXPECT_EQ(state.header.frame_id, "odom");
      EXPECT_FLOAT_EQ(state.state.x, -1.0F);
      EXPECT_FLOAT_EQ(state.state.y, -6.0F);
      EXPECT_FLOAT_EQ(state.state.heading.real, 1.0F);
      EXPECT_FLOAT_EQ(state.state.heading.imag, 0.0F);
      EXPECT_FLOAT_EQ(state.state.longitudinal_velocity_mps, 3.0F);
      EXPECT_FLOAT_EQ(state.state.lateral_velocity_mps, 0.0F);
    }
  }
}

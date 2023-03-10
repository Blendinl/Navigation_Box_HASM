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
// * Added aiim prefix
// * Added aiim namespace
#ifndef CONTROLLER_COMMON_NODES__CONTROLLER_BASE_NODE_HPP_
#define CONTROLLER_COMMON_NODES__CONTROLLER_BASE_NODE_HPP_

#include <aiim_autoware_msgs/msg/control_diagnostic.hpp>
#include <aiim_autoware_msgs/msg/trajectory.hpp>
#include <aiim_autoware_msgs/msg/vehicle_kinematic_state.hpp>
#include <controller_common/controller_base.hpp>
#include <controller_common_nodes/visibility_control.hpp>
#include <tf2/buffer_core.h>
#include <tf2_msgs/msg/tf_message.hpp>

#include <rclcpp/rclcpp.hpp>

#include <exception>
#include <list>
#include <memory>
#include <string>

namespace aiim {
namespace control {
namespace controller_common_nodes {
using controller_common::Command;
using controller_common::Diagnostic;
using controller_common::State;
using controller_common::Trajectory;
using geometry_msgs::msg::TransformStamped;
using tf2_msgs::msg::TFMessage;
using ControllerPtr = std::unique_ptr<controller_common::ControllerBase>;

class CONTROLLER_COMMON_NODES_PUBLIC ControllerBaseNode : public rclcpp::Node {
public:
  /// Parameter file constructor
  ControllerBaseNode(const std::string &name, const std::string &ns);
  /// Explicit constructor
  ControllerBaseNode(const std::string &name, const std::string &ns,
                     const std::string &command_topic,
                     const std::string &state_topic,
                     const std::string &tf_topic,
                     const std::string &trajectory_topic,
                     const std::string &diagnostic_topic,
                     const std::string &static_tf_topic = "static_tf",
                     const float &acceleration_abs_limit = 5.0,
                     const float &steering_abs_limit = 1.0,
                     const float &deceleration_abs_limit = 5.0);

  virtual ~ControllerBaseNode() noexcept = default;

protected:
  /// Child class should call this to set the controller
  void set_controller(ControllerPtr &&controller) noexcept;
  /// Handles errors thrown by either check_new_trajectory(),
  /// handle_new_trajectory() or the std::domain_error from set_trajectory()
  virtual void on_bad_trajectory(std::exception_ptr eptr);
  /// Handles errors thrown by compute_command_impl(), or std::domain_error due
  /// to empty trajectories
  virtual void on_bad_compute(std::exception_ptr eptr);
  /// Expose publishing in case a child class wants to do something during error
  /// handling
  void publish(const Command &msg);

private:

  CONTROLLER_COMMON_NODES_LOCAL void timer_callback();
  rclcpp::TimerBase::SharedPtr timer_;

  // Common initialization
  CONTROLLER_COMMON_NODES_LOCAL void init(const std::string &command_topic,
                                          const std::string &state_topic,
                                          const std::string &tf_topic,
                                          const std::string &static_tf_topic,
                                          const std::string &trajectory_topic,
                                          const std::string &diagnostic_topic,
                                          const float &acceleration_abs_limit = 5.0,
                                          const float &steering_abs_limit = 1.0,
                                          const float &deceleration_abs_limit = 5.0);
  // Callbacks, note passing smart pointers by ref is fine if you're not using
  // ownership semantics: stackoverflow.com/questions/3310737/
  // should-we-pass-a-shared-ptr-by-reference-or-by-value/8741626
  CONTROLLER_COMMON_NODES_LOCAL void on_tf(const TFMessage::SharedPtr &msg);
  CONTROLLER_COMMON_NODES_LOCAL void
  on_static_tf(const TFMessage::SharedPtr &msg);
  CONTROLLER_COMMON_NODES_LOCAL void
  on_trajectory(const Trajectory::SharedPtr &msg);
  CONTROLLER_COMMON_NODES_LOCAL void on_state(const State::SharedPtr &msg);
  // Main computation, false if failure (due to missing tf?)
  CONTROLLER_COMMON_NODES_LOCAL bool try_compute(const State &state);
  // Try to compute control commands from old states in the context of new
  // trajectories and tfs
  CONTROLLER_COMMON_NODES_LOCAL void retry_compute();

  rclcpp::Subscription<State>::SharedPtr m_state_sub{};
  rclcpp::Subscription<TFMessage>::SharedPtr m_tf_sub{};
  rclcpp::Subscription<TFMessage>::SharedPtr m_static_tf_sub{};
  rclcpp::Subscription<Trajectory>::SharedPtr m_trajectory_sub{};
  rclcpp::Publisher<Command>::SharedPtr m_command_pub{};
  rclcpp::Publisher<Diagnostic>::SharedPtr m_diagnostic_pub{};
  tf2::BufferCore m_tf_buffer{tf2::BUFFER_CORE_DEFAULT_CACHE_TIME};
  // TODO(c.ho) diagnostics
  ControllerPtr m_controller{nullptr};
  std::list<State> m_uncomputed_states{};
  // creating the global variables needed for lmiting the controller output
  float accelerationAbsLimit;
  float decelerationAbsLimit;
  float steeringAbsLimit;
  aiim::motion::motion_common::Command cmd;
}; // class ControllerBaseNode
} // namespace controller_common_nodes
} // namespace control
} // namespace aiim

#endif // CONTROLLER_COMMON_NODES__CONTROLLER_BASE_NODE_HPP_

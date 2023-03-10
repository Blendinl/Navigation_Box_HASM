// Copyright 2021 Apex.AI, Inc.
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
// Developed by Apex.AI, Inc.
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
#include <aiim_motion_model/linear_motion_model.hpp>
#include <aiim_state_vector/common_states.hpp>

#include <gtest/gtest.h>

using aiim::motion::motion_model::LinearMotionModel;
using aiim::state_vector::ConstAccelerationXY;
using aiim::state_vector::ConstAccelerationXYYaw;
using aiim::state_vector::variable::X;
using aiim::state_vector::variable::X_ACCELERATION;
using aiim::state_vector::variable::X_VELOCITY;
using aiim::state_vector::variable::Y;
using aiim::state_vector::variable::Y_ACCELERATION;
using aiim::state_vector::variable::Y_VELOCITY;
using aiim::state_vector::variable::YAW;
using aiim::state_vector::variable::YAW_CHANGE_ACCELERATION;
using aiim::state_vector::variable::YAW_CHANGE_RATE;

/// @test Test that prediction on independent x, y, yaw with constant
/// acceleration works.
TEST(LinearMotionModel, PredictConstAccelerationXYYaw) {
  ConstAccelerationXYYaw state{ConstAccelerationXYYaw::Vector::Ones()};
  LinearMotionModel<ConstAccelerationXYYaw> motion_model{};
  state = motion_model.predict(state, std::chrono::milliseconds{100});
  EXPECT_FLOAT_EQ(1.105F, state.at<X>());
  EXPECT_FLOAT_EQ(1.1F, state.at<X_VELOCITY>());
  EXPECT_FLOAT_EQ(1.0F, state.at<X_ACCELERATION>());
  EXPECT_FLOAT_EQ(1.105F, state.at<Y>());
  EXPECT_FLOAT_EQ(1.1F, state.at<Y_VELOCITY>());
  EXPECT_FLOAT_EQ(1.0F, state.at<Y_ACCELERATION>());
  EXPECT_FLOAT_EQ(1.105F, state.at<YAW>());
  EXPECT_FLOAT_EQ(1.1F, state.at<YAW_CHANGE_RATE>());
  EXPECT_FLOAT_EQ(1.0F, state.at<YAW_CHANGE_ACCELERATION>());
}

/// @test Test that prediction on independent x, y with constant acceleration
/// works.
TEST(LinearMotionModel, PredictConstAccelerationXY) {
  ConstAccelerationXY state{ConstAccelerationXY::Vector::Ones()};
  LinearMotionModel<ConstAccelerationXY> motion_model{};
  state = motion_model.predict(state, std::chrono::milliseconds{100});
  EXPECT_FLOAT_EQ(1.105F, state.at<X>());
  EXPECT_FLOAT_EQ(1.1F, state.at<X_VELOCITY>());
  EXPECT_FLOAT_EQ(1.0F, state.at<X_ACCELERATION>());
  EXPECT_FLOAT_EQ(1.105F, state.at<Y>());
  EXPECT_FLOAT_EQ(1.1F, state.at<Y_VELOCITY>());
  EXPECT_FLOAT_EQ(1.0F, state.at<Y_ACCELERATION>());
}

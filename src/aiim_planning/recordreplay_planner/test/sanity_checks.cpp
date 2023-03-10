// Copyright 2020 Embotech AG, Zurich, Switzerland, inspired by Christopher Ho's mpc code
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
#include <aiim_autoware_msgs/msg/trajectory_point.hpp>
#include <aiim_motion_common/config.hpp>
#include <aiim_motion_common/motion_common.hpp>
#include <aiim_motion_testing/motion_testing.hpp>
#include <geometry_msgs/msg/point32.hpp>
#include <recordreplay_planner/recordreplay_planner.hpp>

#include <aiim_autoware_common/types.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <set>
#include <string>

using aiim::common::types::bool8_t;
using aiim::common::types::float32_t;
using aiim::common::types::float64_t;
using aiim::motion::motion_testing::make_state;
using aiim::planning::recordreplay_planner::RecordReplayPlanner;
using aiim_autoware_msgs::msg::Trajectory;
using aiim_autoware_msgs::msg::TrajectoryPoint;
using geometry_msgs::msg::Point32;
using std::chrono::system_clock;

class sanity_checks_base : public ::testing::Test {
protected:
    RecordReplayPlanner planner_{};
};

//------------------ Test basic properties of a recorded, then replayed trajectory
struct PropertyTestParameters {
    std::chrono::milliseconds time_spacing_ms;
    system_clock::time_point starting_time;
};

class sanity_checks_trajectory_properties : public sanity_checks_base,
                                            public testing::WithParamInterface<PropertyTestParameters> {};

TEST_P(sanity_checks_trajectory_properties, basicproperties) {
    const auto p = GetParam();
    auto t0 = p.starting_time;

    // Build a trajectory
    constexpr auto N = 10;
    constexpr auto dx = 1.0F;
    const auto time_increment = p.time_spacing_ms;
    const auto v = dx / (1.0e-3F * p.time_spacing_ms.count());
    for (uint32_t k = {}; k < N; ++k) {
        const auto next_state = make_state(dx * k, 0.0F, 0.0F, v, 0.0F, 0.0F, t0 + k * time_increment);
        planner_.record_state(next_state);
    }

    // Test: Check that the length is equal to the number of states we fed in
    EXPECT_EQ(planner_.get_record_length(), static_cast<std::size_t>(N));

    // Test: Check that the plan returned has the expected time length
    auto trajectory = planner_.plan(make_state(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0));
    float64_t trajectory_time_length =
            trajectory.points[N - 1].time_from_start.sec + 1e-9F * trajectory.points[N - 1].time_from_start.nanosec;
    float64_t endpoint_sec = (1.0F * (N - 1) * time_increment).count() * 1.0e-3;
    float64_t ep = 1.0e-5;
    EXPECT_NEAR(trajectory_time_length, endpoint_sec, ep);
}

INSTANTIATE_TEST_CASE_P(
        trajectory_properties,
        sanity_checks_trajectory_properties,
        testing::Values(
                PropertyTestParameters{std::chrono::milliseconds(100), system_clock::from_time_t({})},
                PropertyTestParameters{std::chrono::milliseconds(200), system_clock::from_time_t({})},
                PropertyTestParameters{std::chrono::milliseconds(100), system_clock::from_time_t(10)},
                PropertyTestParameters{std::chrono::milliseconds(200), system_clock::from_time_t(10)}
                // cppcheck-suppress syntaxError
                ), );

//------------------ Test that length cropping properly works
struct LengthTestParameters {
    // The number of points to be recorded
    uint32_t number_of_points;
};

class sanity_checks_trajectory_length : public sanity_checks_base,
                                        public testing::WithParamInterface<LengthTestParameters> {};

TEST_P(sanity_checks_trajectory_length, length) {
    const auto p = GetParam();
    const auto N = p.number_of_points;
    const auto dummy_state = make_state(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, system_clock::from_time_t({}));

    for (uint32_t k = {}; k < N; ++k) {
        planner_.record_state(dummy_state);
    }

    // Test: Check that the length is equal to the number of states we fed in
    EXPECT_EQ(planner_.get_record_length(), N);
    auto trajectory = planner_.plan(dummy_state);

    EXPECT_EQ(trajectory.points.size(), std::min(N, static_cast<uint32_t>(trajectory.points.max_size())));
}

INSTANTIATE_TEST_CASE_P(
        trajectory_length,
        sanity_checks_trajectory_length,
        testing::Values(LengthTestParameters{80}, LengthTestParameters{200}), );

// Test setup helper function. This creates a planner and records a trajectory
// that goes along the points (0,0), (1,0), .... (N-1,0) with the heading set to
// 0 throughout - for testing purposes
RecordReplayPlanner helper_create_and_record_example(uint32_t N, const std::string outfile="") {
    auto planner = RecordReplayPlanner();
    auto t0 = system_clock::from_time_t({});

    if(outfile != "") {
        planner.start_recording(outfile);
    }

    // Record some states going from
    for (uint32_t k = {}; k < N; ++k) {
        planner.record_state(
                make_state(1.0F * k, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0 + k * std::chrono::milliseconds{100LL}));
    }

    return planner;
}

//------------------ Test that "receding horizon" planning properly works: happy case
TEST(recordreplay_sanity_checks, receding_horizon_happycase) {
    const auto N = 3;
    auto planner = helper_create_and_record_example(N);

    // Call "plan" multiple times in sequence, expecting the states to come back out in order
    const auto t0 = system_clock::from_time_t({});
    for (uint32_t k = {}; k < N; ++k) {
        auto trajectory = planner.plan(make_state(1.0F * k, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0));
        // normally don't check float equality but we _just_ pushed this float so it ought not
        // to have changed
        EXPECT_EQ(1.0F * k, trajectory.points[0].x);
        EXPECT_EQ(N - k, trajectory.points.size());
    }
}

//------------------ Test that "receding horizon" planning properly works:
TEST(recordreplay_sanity_checks, receding_horizon_cornercases) {
    const auto N = 3;
    auto planner = helper_create_and_record_example(N);

    const auto t0 = system_clock::from_time_t({});

    // Check: State we have not recorded, but is closest to the (0,0) state
    {
        auto trajectory = planner.plan(make_state(-1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0));
        EXPECT_EQ(0.0F, trajectory.points[0].x);
    }

    // Check: State we have not recorded, but is closest to the (0,0) state
    {
        auto trajectory = planner.plan(make_state(0.1F, 0.1F, 0.0F, 0.0F, 0.0F, 0.0F, t0));
        EXPECT_EQ(0.0F, trajectory.points[0].x);
        EXPECT_EQ(0.0F, trajectory.points[0].y);
    }

    // Check: State we have not recorded, but is closest to the (N,0) state
    {
        auto trajectory = planner.plan(make_state(1.0F * N + 5.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0));
        EXPECT_EQ((N - 1) * 1.0F, trajectory.points[0].x);
        EXPECT_EQ(0.0F, trajectory.points[0].y);
    }
}

TEST(recordreplay_sanity_checks, state_setting_mechanism) {
    auto planner = RecordReplayPlanner{};

    // Make sure setting and reading the recording state works
    EXPECT_FALSE(planner.is_recording());
    EXPECT_FALSE(planner.is_replaying());

    planner.start_recording();

    EXPECT_TRUE(planner.is_recording());
    EXPECT_FALSE(planner.is_replaying());

    planner.stop_recording();

    EXPECT_FALSE(planner.is_recording());
    EXPECT_FALSE(planner.is_replaying());

    // Make sure setting and reading the replaying state works
    EXPECT_FALSE(planner.is_recording());
    EXPECT_FALSE(planner.is_replaying());

    planner.start_replaying();

    EXPECT_FALSE(planner.is_recording());
    EXPECT_TRUE(planner.is_replaying());

    planner.stop_replaying();

    EXPECT_FALSE(planner.is_recording());
    EXPECT_FALSE(planner.is_replaying());
}

TEST(recordreplay_sanity_checks, heading_weight_setting) {
    auto planner = RecordReplayPlanner{};

    planner.set_heading_weight(0.5);
    EXPECT_EQ(planner.get_heading_weight(), 0.5);
    EXPECT_THROW(planner.set_heading_weight(-1.0), std::domain_error);
}

// Test write/read trajectory to/from file
TEST(RecordreplayWriteReadTrajectory, WriteReadTrajectory) {
    std::string file_name("write_test.trajectory");

    const auto N = 5;
    auto planner = helper_create_and_record_example(N, file_name);

    // Write, clear buffer and read the written data again
    planner.stop_recording();
    planner.clear_record();
    EXPECT_EQ(planner.get_record_length(), static_cast<std::size_t>(0));

    planner.readTrajectoryBufferFromFile(file_name);

    EXPECT_EQ(std::remove(file_name.c_str()), 0);
    EXPECT_EQ(planner.get_record_length(), static_cast<std::size_t>(5));

    const auto t0 = system_clock::from_time_t({});
    auto trajectory = planner.plan(make_state(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, t0));

    for (uint32_t k = {}; k < N; ++k) {
        EXPECT_EQ(1.0F * k, trajectory.points[k].x);
    }
}

TEST(RecordreplayWriteReadTrajectory, readTrajectoryEmptyPath) {
    const auto N = 5;
    auto planner = helper_create_and_record_example(N);

    ASSERT_THROW(planner.readTrajectoryBufferFromFile(""), std::runtime_error);
}

TEST(RecordreplayReachGoal, checkReachGoalCondition) {
    const auto N = 6;
    auto planner = helper_create_and_record_example(N);

    const float64_t distance_thresh = 1.0;
    const float64_t angle_thresh = aiim::common::types::PI_2;

    // vehicle 1.5 meters away from the last point in the trajectory
    {
        const float32_t x = 3.5F;
        const float32_t heading = 0.0F;
        const auto vehicle_state = make_state(x, 0.0F, heading, 0.0F, 0.0F, 0.0F, system_clock::from_time_t({}));
        planner.plan(vehicle_state);
        EXPECT_FALSE(planner.reached_goal(vehicle_state, distance_thresh, angle_thresh));
    }

    // vehicle facing in opposite direction from the last point in the trajectory
    {
        const float32_t x = 5.0F;
        const float32_t heading = -aiim::common::types::PI;
        const auto vehicle_state = make_state(x, 0.0F, heading, 0.0F, 0.0F, 0.0F, system_clock::from_time_t({}));
        planner.plan(vehicle_state);
        EXPECT_FALSE(planner.reached_goal(vehicle_state, distance_thresh, angle_thresh));
    }

    // vehicle state is the same as the last point in the trajectory
    {
        const float32_t x = 5.0F;
        const float32_t heading = 0.0F;
        const auto vehicle_state = make_state(x, 0.0F, heading, 0.0F, 0.0F, 0.0F, system_clock::from_time_t({}));
        planner.plan(vehicle_state);
        EXPECT_TRUE(planner.reached_goal(vehicle_state, distance_thresh, angle_thresh));
    }
}

# Copyright 2020, The Autoware Foundation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Copyright (C) 2021 by NavInfo Europe B.V. The Netherlands - All rights reserved
# Information classification: Confidential
# This content is protected by international copyright laws.
# Reproduction and distribution is prohibited without written permission.
#
# List of changes:
# * Changed package name from autoware_auto_tf2 to aiim_tf2
# * Changed dependencies
cmake_minimum_required(VERSION 3.5)

### Export headers
project(aiim_tf2)

## dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_auto REQUIRED)
ament_auto_find_build_dependencies()


### Test
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # Linters
  ament_lint_auto_find_test_dependencies()
  # Unit test
  ament_add_gtest(test_tf2_aiim_autoware_msgs test/test_tf2_aiim_autoware_msgs.cpp)
  target_include_directories(test_tf2_aiim_autoware_msgs PRIVATE "include")
  ament_target_dependencies(test_tf2_aiim_autoware_msgs
    "aiim_autoware_common"
    "aiim_autoware_msgs"
    "geometry_msgs"
    "orocos_kdl"
    "tf2"
    "tf2_ros"
)
endif()

ament_auto_package()

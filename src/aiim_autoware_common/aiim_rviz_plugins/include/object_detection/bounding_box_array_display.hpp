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
//
// The changes made in this file, of which a summary is listed below, are copyrighted:
// Copyright (C) 2021 by NavInfo Europe B.V. The Netherlands - All rights reserved
// Information classification: Confidential
// This content is protected by international copyright laws.
// Reproduction and distribution is prohibited without written permission.
//
// List of changes:
// * Changed namespace from autoware to aiim
// * Updated dependencies
#pragma once

#include <memory>

#include <aiim_autoware_common/types.hpp>
#include <aiim_autoware_msgs/msg/bounding_box_array.hpp>
#include <rviz_common/display.hpp>
#include <rviz_common/properties/color_property.hpp>
#include <rviz_common/properties/float_property.hpp>
#include <rviz_default_plugins/displays/marker/marker_common.hpp>
#include <rviz_default_plugins/displays/marker_array/marker_array_display.hpp>
#include <visibility_control.hpp>

using aiim::common::types::float32_t;

namespace aiim {
namespace rviz_plugins {

class AIIM_RVIZ_PLUGINS_PUBLIC BoundingBoxArrayDisplay
    : public rviz_common::RosTopicDisplay<aiim_autoware_msgs::msg::BoundingBoxArray> {
    Q_OBJECT

public:
    using MarkerCommon = rviz_default_plugins::displays::MarkerCommon;
    using Marker = visualization_msgs::msg::Marker;
    using BoundingBox = aiim_autoware_msgs::msg::BoundingBox;
    using BoundingBoxArray = aiim_autoware_msgs::msg::BoundingBoxArray;

    BoundingBoxArrayDisplay();
    void onInitialize() override;
    void load(const rviz_common::Config& config) override;
    void update(float32_t wall_dt, float32_t ros_dt) override;
    void reset() override;

private Q_SLOTS:
    void updateProperty();

private:
    // Convert boxes into markers, push them to the display queue
    void processMessage(BoundingBoxArray::ConstSharedPtr array) override;
    // Convert box message to a marker message
    Marker::SharedPtr get_marker(const BoundingBox& box) const;

    std::unique_ptr<MarkerCommon> m_marker_common;
    BoundingBoxArray::ConstSharedPtr msg_cache{};
    rviz_common::properties::ColorProperty* no_label_color_property_;
    rviz_common::properties::ColorProperty* car_color_property_;
    rviz_common::properties::ColorProperty* pedestrian_color_property_;
    rviz_common::properties::ColorProperty* cyclist_color_property_;
    rviz_common::properties::ColorProperty* motorcycle_color_property_;
    rviz_common::properties::ColorProperty* other_color_property_;
    rviz_common::properties::FloatProperty* alpha_property_;
};
}  // namespace rviz_plugins
}  // namespace aiim

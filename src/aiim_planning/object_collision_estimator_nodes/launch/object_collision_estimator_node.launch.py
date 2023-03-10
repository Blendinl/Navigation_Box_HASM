# Copyright 2020 Arm Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import ament_index_python
import launch
import launch_ros.actions


def generate_launch_description():
    """Launch object_collision_estimator_node with default configuration."""
    object_collision_estimator_node = launch_ros.actions.Node(
        package='object_collision_estimator_nodes',
        executable='object_collision_estimator_node_exe',
        name='object_collision_estimator_node',
        output='screen',
        parameters=[
            "{}/params/defaults.param.yaml".format(
                ament_index_python.get_package_share_directory(
                    "object_collision_estimator_nodes"
                )
            ),
        ]
    )

    ld = launch.LaunchDescription([object_collision_estimator_node])
    return ld    

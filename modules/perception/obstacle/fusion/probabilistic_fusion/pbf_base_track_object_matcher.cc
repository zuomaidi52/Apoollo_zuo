/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/perception/obstacle/fusion/probabilistic_fusion/pbf_base_track_object_matcher.h"

#include <numeric>
#include <unordered_map>

namespace apollo {
namespace perception {

double PbfBaseTrackObjectMatcher::s_max_match_distance_ = 4.0;

void PbfBaseTrackObjectMatcher::SetMaxMatchDistance(const double dist) {
  s_max_match_distance_ = dist;
}

double PbfBaseTrackObjectMatcher::GetMaxMatchDistance() {
  return s_max_match_distance_;
}

//-- Zuo: 通过判断fusion_tracks和sensor_objects的交集，将数据重新分配一下
//--   fusion_tracks: 原有的跟踪Obj集合
//--   sensor_objects: 现有的传感器Obj数据
//--   assignments: 交集的元素映射关系
//--   unassigned_fusion_tracks和unassigned_sensor_objects交集以外的两部分
void PbfBaseTrackObjectMatcher::IdAssign(
    const std::vector<PbfTrackPtr> &fusion_tracks,
    const std::vector<std::shared_ptr<PbfSensorObject>> &sensor_objects,
    std::vector<std::pair<int, int>> *assignments,
    std::vector<int> *unassigned_fusion_tracks,
    std::vector<int> *unassigned_sensor_objects) {
  CHECK_NOTNULL(assignments);
  CHECK_NOTNULL(unassigned_fusion_tracks);
  CHECK_NOTNULL(unassigned_sensor_objects);

  size_t num_track = fusion_tracks.size();
  size_t num_obj = sensor_objects.size();

  if (num_track == 0 || num_obj == 0) {
    unassigned_fusion_tracks->resize(num_track);
    unassigned_sensor_objects->resize(num_obj);
    //-- Zuo: 批量递增赋值 ++
    std::iota(unassigned_fusion_tracks->begin(),
              unassigned_fusion_tracks->end(), 0);
    std::iota(unassigned_sensor_objects->begin(),
              unassigned_sensor_objects->end(), 0);
    return;
  }

  const SensorType &sensor_type = sensor_objects[0]->sensor_type;
  const std::string &sensor_id = sensor_objects[0]->sensor_id;

  std::unordered_map<int, int> track_id_2_sensor_id;
  for (size_t i = 0; i < num_track; ++i) {
    //-- Zuo: sensor_type: 传感器类型
    //--      sensor_id:   同类型传感器的不同设备
    //-- 下标映射：fusion_tracks（现有的跟踪集合）中对应（sensor_type, sensor_id）的下标  和  其track_id的下标映射
    //--    用来在下面，判断是否此目标已经被跟踪了
    //-- ？？
    std::shared_ptr<PbfSensorObject> obj =
        fusion_tracks[i]->GetSensorObject(sensor_type, sensor_id);
    if (obj == nullptr) {
      continue;
    }
    //-- Zuo: 将fusion_tracks和sensor_objects是否存在交集，如果有，则将交集的映射关系存储在下
    track_id_2_sensor_id[obj->object->track_id] = i;
  }

  //-- Zuo: 对于fusion_tracks和sensor_objects的交集以外的数据
  //--  分别存放在unassigned_fusion_tracks和unassigned_sensor_objects
  std::vector<bool> fusion_used(num_track, false);
  std::vector<bool> sensor_used(num_obj, false);
  for (size_t i = 0; i < num_obj; ++i) {
    int track_id = sensor_objects[i]->object->track_id;
    auto it = track_id_2_sensor_id.find(track_id);
    if (it != track_id_2_sensor_id.end()) {
      sensor_used[i] = true;
      fusion_used[it->second] = true;
      //-- Zuo: 交集的元素映射关系
      assignments->emplace_back(it->second, i);
    }
  }

  //-- Zuo: unassigned_fusion_tracks == !fusion_used ??
  for (size_t i = 0; i < fusion_used.size(); ++i) {
    if (!fusion_used[i]) {
      unassigned_fusion_tracks->push_back(i);
    }
  }

  //-- Zuo: unassigned_sensor_objects == !sensor_used ??
  for (size_t i = 0; i < sensor_used.size(); ++i) {
    if (!sensor_used[i]) {
      unassigned_sensor_objects->push_back(i);
    }
  }
}

}  // namespace perception
}  // namespace apollo

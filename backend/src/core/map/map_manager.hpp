#pragma once

#include "core/map/map_io.hpp"
#include "core/map/topology_map.hpp"
#include "common/macros.h"

#include <string>
#include <tuple>
#include <vector>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace ros_gui_backend {

using MapOperationResult = std::tuple<bool, std::string>;

class MapManager {
 private:
  std::string ResolveCurrentMapYamlPath() const;

  std::string map_root_;
  std::string frame_id_;
  std::string topo_map_file_name_;
  OccupancyGridData current_map_;
  TopologyMap topo_map_;
  bool map_available_;
  std::string default_tiles_dir_;
  int extra_zoom_levels_{5};
  std::mutex default_map_update_mu_;
  std::condition_variable default_map_update_cv_;
  std::thread default_map_update_worker_;
  bool stop_default_map_update_worker_{false};
  bool has_pending_default_map_update_{false};
  OccupancyGridData wait_handle_default_map_;

  // 上一次真正落盘的默认地图指纹。
  //
  // slam_toolbox 每 map_update_interval(默认 1s) 就重发一次 /map，哪怕地图一个像素
  // 都没变（localization 模式下它本来就是静态的）。而 ProcessDefaultMapUpdate 原本
  // 无条件重写 pgm + 重新生成【全部】瓦片 —— 实测 167x142 的小图要生成 1365 张瓦片、
  // 耗时约 0.75s，也就是常驻吃掉约 3/4 个核，并且每秒把 5.7MB 重写一遍
  // （约 476 GB/天）。开发机上只是浪费，写到机器人的 SD 卡上是会真的写坏卡的。
  //
  // 所以这里记住上次的内容指纹，没变就整个跳过。
  // 只被 worker 线程读写（ProcessDefaultMapUpdate 里），天然无竞争，不用加锁。
  size_t last_default_map_fp_{0};

  // 限流：机器人【移动】时 slam 会持续微调地图（localization 下每秒也变十几个格子），
  // 那时地图真的在变，指纹每帧都不同 → 会退回每秒重生成 1365 张瓦片。
  // 但那个镜像只在 mapping 模式给 save_map.sh 当复制源，导航时根本没人读它。所以移动时
  // 把重生成限到【最多每 5 秒一次】—— 静止(待命/等待)仍是 0 次，移动时降 80%。
  // 代价：导航时 GUI 的实时地图最多滞后 5s 刷新（用户不会盯着看它挪，无感）。
  // 机器人停下后，地图稳定，最多 5s 内镜像收敛到最终状态（save_map.sh 存图前地图早稳了）。
  std::chrono::steady_clock::time_point last_default_map_regen_{};
  static constexpr std::chrono::seconds kDefaultMapMinRegenInterval{5};

  void DefaultMapUpdateWorkerLoop();
  void ProcessDefaultMapUpdate(const OccupancyGridData& data);

 public:
  ~MapManager();

  bool Initialize();

  std::string GetMapRoot() const;
  std::string GetMapDir(const std::string& map_name) const;
  std::string GetTilesDir(const std::string& map_name) const;
  std::string GetDefaultTilesDir() const { return default_tiles_dir_; }
  std::string GetCurrentMapName() const;
  bool SetCurrentMapName(const std::string& map_name);

  std::vector<std::string> ListMapNames() const;
  bool ReadYamlMapMeta(const std::string& yaml_path, double& resolution, double& origin_x,
      double& origin_y, double& origin_yaw, uint32_t& width, uint32_t& height) const;
  bool TryBuildCurrentTilesMetaJson(std::string* out_json) const;

  MapOperationResult ApplyMapEditFromQuery(const std::string& session_id, const std::string& map_name,
      const std::string& source_map_name, const std::string& topology_json,
      const std::string& obstacle_edits_json);
  MapOperationResult ApplyTilesExtraZoomFromJson(const std::string& body_json);
  MapOperationResult ApplyTilesExtraZoomForMapYaml(const std::string& map_name,
      const std::string& body_json);

  LOAD_MAP_STATUS LoadMapFromYaml(const std::string& yaml_file, bool update_current_state = true);
  void UpdateDefaultMap(const OccupancyGridData& data);
  void SetFrameId(const std::string& frame_id) { frame_id_ = frame_id; }
  void SetTopoMapFileName(const std::string& name) { topo_map_file_name_ = name; }
  std::string GetFrameId() const { return frame_id_; }
  std::string GetTopoMapFileName() const { return topo_map_file_name_; }
  bool IsMapAvailable() const { return map_available_; }
  void SetMapAvailable(bool v) { map_available_ = v; }
  void SetExtraZoomLevels(int v) { extra_zoom_levels_ = v; }
  int GetExtraZoomLevels() const { return extra_zoom_levels_; }

  const OccupancyGridData& GetMapData() const { return current_map_; }
  const TopologyMap& GetTopoMap() const { return topo_map_; }

  void RegenerateTiles(const std::string& output_dir);

  DEFINE_SINGLETON(MapManager)
};

}  // namespace ros_gui_backend

#pragma once

#include "core/map/occupancy_grid.hpp"

#include <string>

namespace ros_gui_backend {

class TilesMapGenerator {
public:
  static constexpr int kTileSize = 256;
  static constexpr int kPartitionSize = 128;

  // verbose=false 时不打 "TilesGen: ..." 那几条日志。默认地图镜像频繁重生成，走 false
  // 免得刷屏；用户主动存图走默认 true。
  bool GenerateAllTilesToDir(const OccupancyGridData& map, const std::string& output_dir,
      int extra_zoom_levels, bool verbose = true);
  int GetMaxZoom(uint32_t width, uint32_t height, int extra_zoom_levels) const;
};

}  // namespace ros_gui_backend

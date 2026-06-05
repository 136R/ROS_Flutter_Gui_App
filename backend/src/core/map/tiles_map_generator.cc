#include "core/map/tiles_map_generator.h"

#include "common/config/config.hpp"
#include "common/logger/logger.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <chrono>
#include <cmath>

namespace ros_gui_backend {

namespace {

struct MapTileStyle {
  cv::Vec3b free_bgr;
  cv::Vec3b occ_bgr;
  cv::Vec3b unknown_bgr;
  int free_thresh = 25;
  int occ_thresh = 65;
};

int ClampThresh(int v) {
  return std::max(0, std::min(v, 100));
}

cv::Vec3b ArgbToBgr(int argb) {
  const uint32_t v = static_cast<uint32_t>(argb);
  const uint8_t r = static_cast<uint8_t>((v >> 16) & 0xFF);
  const uint8_t g = static_cast<uint8_t>((v >> 8) & 0xFF);
  const uint8_t b = static_cast<uint8_t>(v & 0xFF);
  return cv::Vec3b(b, g, r);
}

MapTileStyle LoadMapTileStyle() {
  const AppConfig cfg = RootConfig::Instance()->App();
  return {
      ArgbToBgr(cfg.MapTileFreeColor),
      ArgbToBgr(cfg.MapTileOccColor),
      ArgbToBgr(cfg.MapTileUnknownColor),
      ClampThresh(cfg.MapTileFreeThresh),
      ClampThresh(cfg.MapTileOccThresh),
  };
}

cv::Vec3b MapCellToBgr(int8_t cell, const MapTileStyle& style) {
  if (cell < 0 || cell > 100) return style.unknown_bgr;
  if (cell <= style.free_thresh) return style.free_bgr;
  if (cell >= style.occ_thresh) return style.occ_bgr;
  return style.unknown_bgr;
}

}  // namespace

int TilesMapGenerator::GetMaxZoom(uint32_t width, uint32_t height, int extra_zoom_levels) const {
  uint32_t max_dim = std::max(width, height);
  if (max_dim <= kTileSize) return std::max(0, extra_zoom_levels);
  int base = static_cast<int>(std::ceil(std::log2(static_cast<double>(max_dim) / kTileSize)));
  return base + extra_zoom_levels;
}

bool TilesMapGenerator::GenerateAllTilesToDir(const OccupancyGridData& map,
    const std::string& output_dir, int extra_zoom_levels) {
  if (map.width == 0 || map.height == 0 || map.data.empty()) return false;

  int max_z = GetMaxZoom(map.width, map.height, extra_zoom_levels);
  uint32_t padded_w = kTileSize << max_z;
  uint32_t padded_h = kTileSize << max_z;

  double world_min_x = map.origin_x;
  double world_min_y = map.origin_y;
  double world_max_x = map.origin_x + map.width * map.resolution;
  double world_max_y = map.origin_y + map.height * map.resolution;

  LOGGER_INFO(
      "TilesGen: map {}x{} res={} origin=({},{}) extra_zoom_levels={}", map.width, map.height,
      map.resolution, map.origin_x, map.origin_y, extra_zoom_levels);
  LOGGER_INFO("TilesGen: world bounds x=[{},{}] y=[{},{}]", world_min_x, world_max_x, world_min_y,
      world_max_y);
  LOGGER_INFO("TilesGen: max_zoom={} padded={}x{}", max_z, padded_w, padded_h);

  const MapTileStyle tile_style = LoadMapTileStyle();
  int scale = 1 << extra_zoom_levels;

  auto SamplePadded = [&](uint32_t pc, uint32_t pr) -> int8_t {
    if (pc >= padded_w || pr >= padded_h) return -1;
    uint32_t col = pc / scale;
    uint32_t row_inv = pr / scale;
    if (col >= map.width || row_inv >= map.height) return -1;
    uint32_t row = map.height - 1 - row_inv;
    return map.data[row * map.width + col];
  };

  namespace fs = boost::filesystem;
  const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
  const fs::path out_path(output_dir);
  const fs::path out_parent = out_path.parent_path().empty() ? fs::path(".") : out_path.parent_path();
  const std::string out_name = out_path.filename().string();
  const fs::path staging_dir = out_parent / (out_name + ".staging." + std::to_string(now_ms));
  const fs::path backup_dir = out_parent / (out_name + ".bak." + std::to_string(now_ms));

  fs::create_directories(out_parent);
  if (fs::exists(staging_dir)) {
    fs::remove_all(staging_dir);
  }
  if (fs::exists(backup_dir)) {
    fs::remove_all(backup_dir);
  }
  fs::create_directories(staging_dir);

  int total = 0;
  for (int z = 0; z <= max_z; ++z) {
    int tiles_per_axis = 1 << z;
    int tile_src_size = kTileSize << (max_z - z);

    std::string tile_list;
    for (int x = 0; x < tiles_per_axis; ++x) {
      for (int y = 0; y < tiles_per_axis; ++y) {
        int src_x0 = x * tile_src_size;
        int src_y0 = y * tile_src_size;

        cv::Mat tile(kTileSize, kTileSize, CV_8UC3);
        for (int ty = 0; ty < kTileSize; ++ty) {
          for (int tx = 0; tx < kTileSize; ++tx) {
            int sx = src_x0 + tx * tile_src_size / kTileSize;
            int sy = src_y0 + ty * tile_src_size / kTileSize;
            sx = std::max(0, std::min(sx, static_cast<int>(padded_w) - 1));
            sy = std::max(0, std::min(sy, static_cast<int>(padded_h) - 1));
            int8_t cell = SamplePadded(static_cast<uint32_t>(sx), static_cast<uint32_t>(sy));
            tile.at<cv::Vec3b>(ty, tx) = MapCellToBgr(cell, tile_style);
          }
        }

        fs::path dir = staging_dir / std::to_string(z) / std::to_string(x);
        fs::create_directories(dir);
        std::string path = (dir / (std::to_string(y) + ".png")).string();
        if (!cv::imwrite(path, tile)) return false;
        ++total;

        if (!tile_list.empty()) tile_list += " ";
        tile_list += std::to_string(z) + "/" + std::to_string(x) + "/" + std::to_string(y);
      }
    }
    // LOG_INFO("TilesGen: z=" << z << " tiles_per_axis=" << tiles_per_axis
    //     << " x=[0," << (tiles_per_axis - 1) << "] y=[0," << (tiles_per_axis - 1) << "] "
    //     << tile_list);
  }
  if (fs::exists(out_path)) {
    fs::rename(out_path, backup_dir);
  }
  fs::rename(staging_dir, out_path);
  if (fs::exists(backup_dir)) {
    try {
      fs::remove_all(backup_dir);
    } catch (const std::exception& e) {
      LOGGER_WARN("TilesGen: cleanup backup dir failed {}, err={}", backup_dir.string(), e.what());
    }
  }
  LOGGER_INFO("TilesGen: total {} tiles written to {}", total, output_dir);
  return total > 0;
}

}  // namespace ros_gui_backend

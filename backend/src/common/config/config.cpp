#include "common/config/config.hpp"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

namespace ros_gui_backend {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SshQuickCommandEntry, name, cmd, use_sudo)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    AppConfig,
    MapManagerFrameId,
    MapPubTopic,
    MapSubTopic,
    NavToPoseStatusTopic,
    NavThroughPosesStatusTopic,
    LaserTopic,
    LocalPathTopic,
    GlobalPathTopic,
    TracePathTopic,
    OdomTopic,
    BatteryTopic,
    RobotFootprintTopic,
    LocalCostmapTopic,
    GlobalCostmapTopic,
    PointCloud2Topic,
    DiagnosticTopic,
    RelocTopic,
    NavGoalTopic,
    SpeedCtrlTopic,
    MapFrameName,
    BaseLinkFrameName,
    TopologyLiveTopic,
    TopologyJsonTopic,
    TopologyPublishTopic,
    SSHHost,
    SSHPort,
    SSHUsername,
    SSHPassword,
    SSHQuickCommands,
    MapTileFreeColor,
    MapTileOccColor,
    MapTileUnknownColor,
    MapTileFreeThresh,
    MapTileOccThresh)

void SetAppConfigStoragePath(std::string path) {
  RootConfig::Instance()->SetStoragePath(std::move(path));
}

std::string ResolvedAppConfigPath() {
  return RootConfig::Instance()->ResolvedStoragePath();
}

RootConfig::RootConfig() = default;

void RootConfig::SetStoragePath(std::string path) {
  storage_path_ = std::move(path);
}

std::string RootConfig::ResolvedStoragePath() const {
  if (!storage_path_.empty()) {
    return storage_path_;
  }
  return (fs::current_path() / "gui_app_settings.json").string();
}

void AppConfigToJson(const AppConfig& s, nlohmann::json* out) {
  *out = s;
}

void AppConfigMergeJson(const nlohmann::json& j, AppConfig* s) {
  nlohmann::json merged = *s;
  merged.merge_patch(j);
  *s = merged.get<AppConfig>();
}

namespace {

int GrayToArgb(int gray) {
  const int g = std::max(0, std::min(gray, 255));
  return 0xFF000000 | (g << 16) | (g << 8) | g;
}

void MigrateLegacyMapTileGray(const nlohmann::json& j, AppConfig* s) {
  if (!j.contains("MapTileFreeColor") && j.contains("MapTileFreeGray")) {
    s->MapTileFreeColor = GrayToArgb(j["MapTileFreeGray"].get<int>());
  }
  if (!j.contains("MapTileOccColor") && j.contains("MapTileOccGray")) {
    s->MapTileOccColor = GrayToArgb(j["MapTileOccGray"].get<int>());
  }
  if (!j.contains("MapTileUnknownColor") && j.contains("MapTileUnknownGray")) {
    s->MapTileUnknownColor = GrayToArgb(j["MapTileUnknownGray"].get<int>());
  }
}

}  // namespace

bool LoadAppConfigFile(AppConfig* s) {
  const std::string path = RootConfig::Instance()->ResolvedStoragePath();
  std::ifstream ifs(path);
  if (!ifs) {
    return true;
  }
  std::string raw((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
  try {
    nlohmann::json j = nlohmann::json::parse(raw);
    AppConfigMergeJson(j, s);
    MigrateLegacyMapTileGray(j, s);
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

bool SaveAppConfigFile(const AppConfig& s) {
  const std::string path = RootConfig::Instance()->ResolvedStoragePath();
  nlohmann::json j;
  AppConfigToJson(s, &j);
  std::ofstream ofs(path);
  if (!ofs) {
    return false;
  }
  ofs << j.dump(2);
  return true;
}

}  // namespace ros_gui_backend

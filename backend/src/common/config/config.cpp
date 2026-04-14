#include "common/config/config.hpp"

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
    SSHQuickCommands)

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

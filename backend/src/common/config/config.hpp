#pragma once

#include "common/macros.h"
#include "core/map/json.hpp"

#include <string>
#include <utility>
#include <vector>

namespace ros_gui_backend {

struct SshQuickCommandEntry {
  std::string name;
  std::string cmd;
  bool use_sudo = false;
};

struct AppConfig {
  std::string MapManagerFrameId{"map"};
  std::string MapPubTopic{"/map"};
  std::string MapSubTopic{"/map"};
  std::string NavToPoseStatusTopic{"/navigate_to_pose/_action/status"};
  std::string NavThroughPosesStatusTopic{"/navigate_through_poses/_action/status"};
  std::string LaserTopic{"/scan"};
  std::string LocalPathTopic{"/local_plan"};
  std::string GlobalPathTopic{"/plan"};
  std::string TracePathTopic{"/transformed_global_plan"};
  std::string OdomTopic{"/wheel/odometry"};
  std::string BatteryTopic{"/battery_status"};
  std::string RobotFootprintTopic{"/local_costmap/published_footprint"};
  std::string LocalCostmapTopic{"/local_costmap/costmap"};
  std::string GlobalCostmapTopic{"/global_costmap/costmap"};
  std::string PointCloud2Topic{"points"};
  std::string DiagnosticTopic{"/diagnostics"};
  std::string RelocTopic{"/initialpose"};
  std::string NavGoalTopic{"/goal_pose"};
  std::string SpeedCtrlTopic{"/cmd_vel"};
  std::string MapFrameName{"map"};
  std::string BaseLinkFrameName{"base_link"};
  std::string TopologyLiveTopic{"/map/topology"};
  std::string TopologyJsonTopic{};
  std::string TopologyPublishTopic{"/map/topology/update"};
  std::string SSHHost;
  int SSHPort = 22;
  std::string SSHUsername;
  std::string SSHPassword;
  std::vector<SshQuickCommandEntry> SSHQuickCommands;
};

void AppConfigToJson(const AppConfig& s, nlohmann::json* out);
void AppConfigMergeJson(const nlohmann::json& j, AppConfig* s);

void SetAppConfigStoragePath(std::string path);
std::string ResolvedAppConfigPath();

bool LoadAppConfigFile(AppConfig* s);
bool SaveAppConfigFile(const AppConfig& s);

class RootConfig {
 public:
  void SetStoragePath(std::string path);
  std::string ResolvedStoragePath() const;

  const AppConfig& App() const { return app_config_; }
  AppConfig& MutableApp() { return app_config_; }

 private:
  std::string storage_path_;
  AppConfig app_config_;

  DEFINE_SINGLETON(RootConfig)
};

}  // namespace ros_gui_backend

#include "node/ros1/node.hpp"
#include "node/ros1/convert.hpp"
#include "common/logger/logger.h"

#include <stdexcept>

namespace ros_gui_backend {

RosGuiNode::RosGuiNode() {}

RosGuiNode::~RosGuiNode() {}

bool RosGuiNode::Init(const AppConfig& app_config) {
  (void)app_config;
  map_pub_ = nh_.advertise<nav_msgs::OccupancyGrid>("/map_manager/map", 1, true);
  get_map_service_ = nh_.advertiseService("static_map", &RosGuiNode::GetMapCallback, this);
  return true;
}

void RosGuiNode::Run() {
  ros::spin();
}

void RosGuiNode::Shutdown() {
  ros::shutdown();
}

bool RosGuiNode::SetRobotStreamImageSubscription(
    const std::string& topic, bool subscribe, std::string* error_message) {
  (void)topic;
  (void)subscribe;
  (void)error_message;
  return true;
}

bool RosGuiNode::ReloadGuiStreams(const AppConfig& settings) {
  (void)settings;
  return false;
}

bool RosGuiNode::PublishCmdVel(double vx, double vy, double vw) {
  (void)vx;
  (void)vy;
  (void)vw;
  return false;
}

bool RosGuiNode::PublishNavGoal(double x, double y, double roll, double pitch, double yaw) {
  (void)x;
  (void)y;
  (void)roll;
  (void)pitch;
  (void)yaw;
  return false;
}

bool RosGuiNode::PublishInitialPose(double x, double y, double roll, double pitch, double yaw) {
  (void)x;
  (void)y;
  (void)roll;
  (void)pitch;
  (void)yaw;
  return false;
}

bool RosGuiNode::PublishNavCancel() {
  return false;
}

bool RosGuiNode::PublishMap(const OccupancyGridData& map, const std::string& frame_id) {
  {
    std::lock_guard<std::mutex> lock(map_mu_);
    map_data_ = map;
    map_frame_id_ = frame_id;
    map_available_ = true;
  }
  nav_msgs::OccupancyGrid msg;
  Convert(map, msg, frame_id);
  map_pub_.publish(msg);
  return true;
}

bool RosGuiNode::LookupTransform(const std::string& target_frame, const std::string& source_frame,
    std::string* json_out, std::string* err) {
  (void)target_frame;
  (void)source_frame;
  (void)json_out;
  if (err) {
    *err = "ROS1 backend: transform API not implemented";
  }
  return false;
}

void RosGuiNode::GetMapCallback(nav_msgs::GetMap::Request&, nav_msgs::GetMap::Response& res) {
  std::lock_guard<std::mutex> lock(map_mu_);
  if (map_available_) {
    Convert(map_data_, res.map, map_frame_id_);
  }
}

}  // namespace ros_gui_backend

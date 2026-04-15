#include "core/robot_stream/robot_ws_controller.hpp"

#include "core/robot_stream/robot_message_hub.hpp"
#include "common/logger/logger.h"
#include "node/node_manager.hpp"
#include "robot_message.pb.h"

#include <drogon/WebSocketConnection.h>

namespace ros_gui_backend {

void RobotWsController::handleNewMessage(const drogon::WebSocketConnectionPtr& conn,
    std::string&& message, const drogon::WebSocketMessageType& type) {
  if (type != drogon::WebSocketMessageType::Binary) {
    return;
  }
  pb::ClientRobotMessage msg;
  if (!msg.ParseFromString(message)) {
    LOGGER_WARN("robot ws decode failed from {}", conn->peerAddr().toIpPort());
    return;
  }
  auto node = NodeManager::Instance()->GetNode();
  if (!node) {
    LOGGER_WARN("robot ws message ignored: ros node not ready");
    return;
  }
  switch (msg.payload_case()) {
    case pb::ClientRobotMessage::kCmdVel: {
      const auto& tw = msg.cmd_vel();
      const double vx = tw.linear().x();
      const double vy = tw.linear().y();
      const double vw = tw.angular().z();
      if (!node->PublishCmdVel(vx, vy, vw)) {
        LOGGER_WARN("robot ws cmd_vel publish failed");
      }
      break;
    }
    default:
      break;
  }
}

void RobotWsController::handleNewConnection(
    const drogon::HttpRequestPtr&, const drogon::WebSocketConnectionPtr& conn) {
  LOGGER_INFO("robot ws connected: {}", conn->peerAddr().toIpPort());
  RobotMessageHub::Instance().AddClient(conn);
}

void RobotWsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) {
  LOGGER_INFO("robot ws disconnected: {}", conn->peerAddr().toIpPort());
  RobotMessageHub::Instance().RemoveClient(conn);
}

}  // namespace ros_gui_backend

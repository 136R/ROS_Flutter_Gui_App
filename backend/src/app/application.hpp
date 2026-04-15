#pragma once

#include "common/config/config.hpp"

#include <memory>
#include <string>

namespace ros_gui_backend {

class WebServer;

class Application {
 public:
  Application();
  ~Application();

  bool Initialize(int argc, char** argv);
  int Start();
  void Stop();

 private:
  int argc_{0};
  char** argv_{nullptr};
  std::string config_json_path_;
  std::string default_map_yaml_path_;
  int web_server_port_{8080};
  std::string web_server_document_root_;
  AppConfig settings_;
  std::unique_ptr<WebServer> web_server_;
  bool initialized_{false};
  bool ros_runtime_inited_{false};
  bool stopped_{false};
};

}  // namespace ros_gui_backend

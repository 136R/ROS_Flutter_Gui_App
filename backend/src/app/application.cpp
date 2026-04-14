#include "app/application.hpp"

#include "core/map/map_manager.hpp"
#include "common/config/config.hpp"
#include "core/web_server/web_server.hpp"
#include "common/logger/logger.h"
#include "node/node_manager.hpp"

#include <boost/program_options.hpp>
#include <cstring>
#include <filesystem>
namespace ros_gui_backend {

namespace {

namespace bpo = boost::program_options;

bpo::variables_map ParseCliOptions(int argc, char** argv) {
  bpo::options_description desc("ros_gui_backend options");
  desc.add_options()
      ("config-json", bpo::value<std::string>(), "config json path")
      ("config", bpo::value<std::string>(), "compat alias of config-json")
      ("port", bpo::value<int>(), "web server port")
      ("document-root", bpo::value<std::string>(), "web static document root");
  bpo::variables_map vm;
  bpo::store(bpo::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
  bpo::notify(vm);
  return vm;
}

}  // namespace

Application::Application() = default;

Application::~Application() {
  Stop();
}

bool Application::Initialize(int argc, char** argv) {
  argc_ = argc;
  argv_ = argv;
  config_json_path_ = ParseConfigJsonPathFromArgs(argc, argv);
  web_server_port_ = ParseWebServerPortFromArgs(argc, argv);
  web_server_document_root_ = ParseWebServerDocumentRootFromArgs(argc, argv);
  initialized_ = true;
  return true;
}

std::string Application::ParseConfigJsonPathFromArgs(int argc, char** argv) const {
  const bpo::variables_map vm = ParseCliOptions(argc, argv);
  if (vm.count("config-json") > 0) {
    return vm["config-json"].as<std::string>();
  }
  if (vm.count("config") > 0) {
    return vm["config"].as<std::string>();
  }
  return std::string();
}

int Application::ParseWebServerPortFromArgs(int argc, char** argv) const {
  const bpo::variables_map vm = ParseCliOptions(argc, argv);
  if (vm.count("port") == 0) {
    return 8080;
  }
  const int p = vm["port"].as<int>();
  if (p >= 1 && p <= 65535) {
    return p;
  }
  return 8080;
}

std::string Application::ParseWebServerDocumentRootFromArgs(int argc, char** argv) const {
  const bpo::variables_map vm = ParseCliOptions(argc, argv);
  if (vm.count("document-root") > 0) {
    return vm["document-root"].as<std::string>();
  }
  return std::string();
}

void Application::Stop() {
  if (stopped_) {
    return;
  }
  stopped_ = true;
}

int Application::Start() {
  if (!initialized_) {
    LOGGER_ERROR("Application not initialized");
    return -1;
  }
  stopped_ = false;

  namespace fs = std::filesystem;
  const std::string gui_json_path =
      config_json_path_.empty() ? (fs::current_path() / "gui_app_settings.json").string() : config_json_path_;
  SetAppConfigStoragePath(gui_json_path);
  RootConfig::Instance()->MutableApp() = AppConfig{};
  if (!LoadAppConfigFile(&RootConfig::Instance()->MutableApp())) {
    LOGGER_WARN("gui_app_settings.json invalid at {}", gui_json_path);
  }
  settings_ = RootConfig::Instance()->App();

  MapManager* mm = MapManager::Instance();
  mm->SetFrameId(settings_.MapManagerFrameId);
  if (!mm->Initialize()) {
    LOGGER_ERROR("Failed to initialize map manager");
    Stop();
    return -1;
  }

  if (!NodeManager::Instance()->InitNode()) {
    Stop();
    return -1;
  }

  WebServer::SetSigintHook([]() {  NodeManager::Instance()->GetNode()->Shutdown(); });

  web_server_ = std::make_unique<WebServer>();
  WebServerConfig web_server_cfg;
  web_server_cfg.port = web_server_port_;
  web_server_cfg.document_root = web_server_document_root_;
  if (!web_server_->Start(web_server_cfg)) {
    Stop();
    return -1;
  }

  NodeManager::Instance()->GetNode()->Run();

  WebServer::ClearSigintHook();
  if (web_server_) {
    web_server_->Shutdown();
    web_server_.reset();
  }
  NodeManager::Instance()->Reset();

  stopped_ = true;
  return 0;
}

}  // namespace ros_gui_backend

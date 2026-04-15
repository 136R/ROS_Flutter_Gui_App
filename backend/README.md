# 后端（ros_gui_backend）

[English](README_EN.md)

部署在机器人侧的单进程服务：**ROS1 / ROS2** 节点与 **HTTP（Drogon）**、**WebSocket 机器人数据流** 同进程运行，为 Flutter/Web 等上位机提供瓦片地图、地图管理 REST、GUI 话题配置持久化等能力。默认地图数据目录为 **`~/.maps`**（无 `HOME` 时为 **`./.maps`**）。

---

## 1. 编译与依赖

### 1.1 系统依赖（示例：Ubuntu）

```bash
sudo apt-get install libsdl2-dev libsdl2-image-dev protobuf-compiler libc-ares-dev -y
```

- 已安装 **ROS1** 或 **ROS2**，编译与运行前需 `source /opt/ros/<distro>/setup.bash`。
- CMake 会通过 `cmake/` 下脚本拉取 **yaml-cpp**、**fmt**、**Drogon** 等。

### 1.2 自仓库根目录一键构建（推荐）

在 **`nav2_map_server` 仓库根目录**：

```bash
./build.sh
```

将依次：**protocol → Dart protobuf**、**本 backend CMake 构建**、**Flutter Web**。仅需后端时可在 `backend/` 下单独 `cmake`（见下一节）。

### 1.3 仅构建本目录

```bash
cd backend
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build --parallel
```

切换 ROS 版本：

```bash
cmake -S . -B build -DROS_VERSION=1   # ROS1，默认 ROS2
```

国内镜像等参数可透传给 CMake；仓库内另有 `backend/build_cn.sh` 等脚本时可按需使用。

### 1.4 安装产物

- 可执行文件：**`backend/build/install/bin/ros_gui_backend`**
- 启动脚本与示例配置：**`backend/build/install/bin/start.sh`**、**`backend/build/install/bin/cfg/config.yaml`**

---

## 2. 运行

运行前 **`source`** 对应 ROS 环境。

### 2.1 推荐方式

```bash
cd backend/build/install/bin
./start.sh
```

`start.sh` 会设置 **`LD_LIBRARY_PATH=../lib:...`** 后执行：

`./ros_gui_backend --config-json ./gui_app_settings.json --port 8080 --document-root ./dist`

### 2.2 直接启动

```bash
cd backend/build/install/bin
export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH
./ros_gui_backend --config-json ./gui_app_settings.json --port 8080 --document-root ./dist
```

### 2.3 命令行参数

| 参数 | 说明 |
| --- | --- |
| `--config-json <path>` | AppConfig JSON 配置文件路径（默认 `./gui_app_settings.json`） |
| `--config <path>` | 兼容别名，等同于 `--config-json` |
| `--port <1-65535>` | HTTP 服务端口（默认 `8080`） |
| `--default-map <path/to/map.yaml>` | 启动时加载指定地图 YAML，并写入 `~/.maps/map` 作为默认地图 |
| `--document-root <path>` | 静态资源目录（未指定时默认 `<executable_dir>/dist`） |

未指定 **`--config-json`** 时，默认读取当前目录下的 **`gui_app_settings.json`**。  
`--port` 和 `--document-root` 由命令行参数决定，不再由 JSON 配置管理。

---

## 3. 进程与源码结构

```
main.cpp
└── Application
    ├── MapManager（单例）
    │   ├── 地图根目录 ~/.maps、当前图、YAML/PGM、拓扑 JSON、瓦片目录
    │   └── 与 map_io、tiles 生成逻辑协作
    ├── WebServer（Drogon）
    │   ├── /tiles/*、地图列表、切换、删除、拓扑、编辑、/api/settings、/robot/* 等
    │   └── WebSocket /ws/robot（protobuf RobotMessage 流）
    └── RosGuiNode
        ├── ros2/ 或 ros1/ 实现
        └── 订阅激光/路径/里程计等，推送到 WS；ROS2 下订阅 /map 更新内存图并提供 Nav2 相关服务
```

| 路径 | 职责 |
| --- | --- |
| `src/app/` | `main`、`Application`、YAML 加载 |
| `src/node/` | ROS 节点适配、`node_config.hpp` |
| `src/core/` | 地图核心、`web_server`、robot_stream、瓦片生成 |
| `src/common/` | 日志、`config/`（GuiAppSettings 持久化等） |

**`MapManager::Initialize()`**：若 `~/.maps/current_map` 指向的目录下存在 **`<当前地图名>.yaml`**，则尝试加载；否则可在运行后通过 ROS2 订阅 **`/map`** 等更新内存图。

---

## 4. 地图数据目录约定

```
~/.maps/
├── current_map              # 文本：当前地图目录名
├── <地图名>/
│   ├── <地图名>.yaml
│   ├── <地图名>.pgm
│   ├── <地图名>.topology
│   ├── map.topology         # GET /getTopologyMap?map_name= 等
│   └── tiles/               # 瓦片 {z}/{x}/{y}.png
└── ...
```

实际启动时，后端程序会订阅/map话题存放至~/.maps/map，当用户在前端切换地图时，后端会将当前切换后的新地图发送至/map话题



---

## 5. HTTP 接口摘要

默认监听 **`0.0.0.0:<端口>`**（`web_server.port`，默认 **8080**）。已注册 **CORS**（含 `OPTIONS`）。

### 5.1 瓦片与元数据

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/tiles/{z}/{x}/{y}` | 当前地图瓦片 PNG |
| GET | `/tiles/{map_name}/{z}/{x}/{y}` | 指定地图瓦片 |
| GET | `/tiles/meta` | 当前图瓦片元信息 JSON |
| GET | `/tiles/{map_name}/meta` | 指定图元信息 |
| POST | `/tiles/config` | JSON：`extra_zoom_levels` |
| POST | `/tiles/{map_name}/config` | 同上 |
| GET | `/tiles/` | 接口说明文本 |

路径末尾常加 **`.png`**，需与路由或反向代理一致。

### 5.2 地图管理

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/getAllMapList` | 地图名 JSON 数组 |
| GET | `/currentMap` | `{"current_map":"..."}` |
| GET | `/setCurrentMap?name=` | 切换当前图 |
| GET | `/deleteMap?map_name=` | 删除地图目录 |
| GET | `/getTopologyMap` | 当前内存拓扑 |
| GET | `/getTopologyMap?map_name=` | 从磁盘读拓扑 |

### 5.3 编辑与其它

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/saveMapEdit` | Query：`session_id`、`map_name`、`topology_json`、`obstacle_edits_json`（URL 编码） |
| GET | `/api/settings` | GUI 话题/坐标系等 JSON |
| POST | `/api/settings` | 合并写入并触发 ROS 流重绑 |
| POST | `/robot/nav_goal`、`/robot/initial_pose` 等 | 由前端调用的控制接口（`cmd_vel` 已改为 WebSocket 二进制消息） |

完整列表以实现代码 **`web_server.cpp`** 为准。

---

## 6. ROS 接口

节点名：**`ros_gui_app_backend`**。

### 6.1 ROS2

| 类型 | 名称 | 说明 |
| --- | --- | --- |
| 发布 | **`/map_manager/map`**（源码常量） | `nav_msgs/OccupancyGrid`，transient local |
| 订阅 | **`/map`**（源码常量） | 更新内存图与瓦片相关数据 |
| 服务 | `ros_gui_app_backend/map` | `nav_msgs/srv/GetMap` |
| 服务 | `ros_gui_app_backend/load_map` | `nav2_msgs/srv/LoadMap` |
| 服务 | `ros_gui_app_backend/save_map` | `nav2_msgs/srv/SaveMap` |

**SaveMap** 临时订阅的 QoS、超时秒数、默认 `free_thresh` / `occupied_thresh` 由 **`node/ros2/node.cpp`** 内常量决定（与此前 YAML 默认值一致），不再通过配置文件覆盖。

### 6.2 ROS1

| 类型 | 名称 | 说明 |
| --- | --- | --- |
| 发布 | **`/map_manager/map`**（源码常量） | `nav_msgs/OccupancyGrid` |
| 服务 | **`/static_map`** | `nav_msgs/GetMap` |

ROS1 实现**未**订阅 `/map` 回写内存图；差异以 **`src/node/ros1/`** 为准。

---

## 7. 配置文件

安装目录 **`bin/cfg/config.yaml`** 由 **`Application`** 解析：

- **`map_manager`**：`frame_id` 等  
- **`ros_gui_node`**：`map_frame`、`base_link_frame` 及各 GUI/ROS 话题名字段（**`NodeConfig`**，写入后与 **`gui_app_settings.json`** / **`GuiAppSettings`** 合并）；地图发布订阅名与 SaveMap 默认阈值等已固定在源码中  
- **`web_server`**：`port`

---

## 8. 第三方与说明

- **HTTP**：Drogon  
- **JSON / 配置**：nlohmann/json、yaml-cpp、fmt  
- **图像与瓦片**：OpenCV、SDL2 / SDL2_image、Boost.Filesystem  

构建**不**将 `topology_msgs` 等作为硬依赖；拓扑以磁盘 JSON + HTTP 为主与上位机交互。

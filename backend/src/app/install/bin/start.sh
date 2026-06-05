export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH

# App 配置仅使用当前目录下的 gui_app_settings.json（由 /api/settings 读写）
./ros_gui_backend --config-json ./gui_app_settings.json --port 8080 --document-root ./dist
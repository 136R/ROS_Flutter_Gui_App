export LD_LIBRARY_PATH=../lib:$LD_LIBRARY_PATH

./ros_gui_backend --config-json ./gui_app_settings.json --port 8080 --document-root ./dist
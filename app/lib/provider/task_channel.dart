import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:ros_flutter_gui_app/basic/task_status.dart';
import 'package:ros_flutter_gui_app/global/setting.dart';

/// 移动垃圾桶任务层（ROS 侧的 my_bot_task）的通道。
///
/// 【为什么不走后端的 WebSocket】
/// 后端是【固定 schema 的 protobuf 桥】，只推激光/地图/里程计那几种消息，
/// 消费不了 /task/status 这种自定义类型 —— 加一个类型要同时改 C++ 后端、.proto
/// 和 Dart 生成代码，比直接 HTTP 贵得多。
///
/// 所以任务层自己开了一个 HTTP 口（默认 :8090），这里 1Hz 轮询。召唤是低频离散
/// 动作，不需要 WebSocket。
class TaskChannel {
  /// 当前任务状态。null = 连不上任务层（没起 my_bot_task，或端口填错）。
  final ValueNotifier<TaskStatus?> status = ValueNotifier(null);

  Timer? _timer;
  bool _inFlight = false;

  void start() {
    _timer?.cancel();
    _poll();
    _timer = Timer.periodic(const Duration(seconds: 1), (_) => _poll());
  }

  void stop() {
    _timer?.cancel();
    _timer = null;
    status.value = null;
  }

  Future<void> _poll() async {
    if (_inFlight) return; // 上一发还没回来就跳过，别堆积
    _inFlight = true;
    try {
      final r = await http
          .get(Uri.parse('${globalSetting.taskServerUrl}/task/status'))
          .timeout(const Duration(seconds: 2));
      if (r.statusCode == 200) {
        status.value = TaskStatus.fromJson(
            jsonDecode(utf8.decode(r.bodyBytes)) as Map<String, dynamic>);
      } else {
        status.value = null;
      }
    } catch (_) {
      // 任务层没起、端口不通都会走到这里。静默失败 —— 任务卡整张隐藏就是了，
      // 不要弹 toast 刷屏。
      status.value = null;
    } finally {
      _inFlight = false;
    }
  }

  /// 「我倒完了，回去吧」—— 跳过等待，立刻归位。
  Future<bool> skipWait() => _post('/task/skip');

  /// 清空整个召唤队列。
  Future<bool> cancelAll() => _post('/task/cancel_all');

  Future<bool> _post(String path) async {
    try {
      final r = await http
          .post(Uri.parse('${globalSetting.taskServerUrl}$path'))
          .timeout(const Duration(seconds: 3));
      if (r.statusCode == 200) {
        await _poll(); // 立刻刷新，别等下一个轮询周期，否则按钮像没反应
        return true;
      }
    } catch (_) {}
    return false;
  }
}

/// my_bot_task 的 GET /task/status 返回的东西。
///
/// 契约定义在 dev_ws/src/my_bot_task/docs/README.md。字段变了要两边一起改。
class TaskStatus {
  /// IDLE / NAVIGATING / WAITING / RETURNING / STOPPED
  final String state;

  /// 正在服务的房间（RETURNING 时是待命点）。IDLE/STOPPED 时为 null。
  final String? currentName;

  /// 排队中的房间，先来先到。
  final List<String> queue;

  /// WAITING 时：还剩多少秒自动归位。
  final double dwellRemaining;

  /// Nav2 给的剩余【路径长度】(m) —— 真实数据。
  final double distanceRemaining;

  /// 任务层折算的预计到达时间(s)。0 = 还没收到第一帧导航反馈（显示"计算中"）。
  final double etaSec;

  final String lastError;

  const TaskStatus({
    required this.state,
    required this.currentName,
    required this.queue,
    required this.dwellRemaining,
    required this.distanceRemaining,
    required this.etaSec,
    required this.lastError,
  });

  factory TaskStatus.fromJson(Map<String, dynamic> j) {
    final cur = j['current'];
    return TaskStatus(
      state: (j['state'] ?? 'IDLE') as String,
      currentName: cur is Map ? cur['name'] as String? : null,
      queue: ((j['queue'] ?? []) as List).map((e) => e.toString()).toList(),
      dwellRemaining: (j['dwell_remaining'] as num?)?.toDouble() ?? 0,
      distanceRemaining: (j['distance_remaining'] as num?)?.toDouble() ?? 0,
      etaSec: (j['eta_sec'] as num?)?.toDouble() ?? 0,
      lastError: (j['last_error'] ?? '') as String,
    );
  }

  bool get isIdle => state == 'IDLE';
  bool get isNavigating => state == 'NAVIGATING';
  bool get isWaiting => state == 'WAITING';
  bool get isReturning => state == 'RETURNING';
  bool get isStopped => state == 'STOPPED';

  /// 待命时整张任务卡都不显示 —— 地图保持干净。
  bool get shouldShowCard => !isIdle;
}

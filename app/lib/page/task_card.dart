import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:ros_flutter_gui_app/basic/task_status.dart';
import 'package:ros_flutter_gui_app/provider/task_channel.dart';

/// 浮在地图页底部居中的「任务卡」。
///
/// 待命(IDLE)时整张隐藏，地图保持干净；只有 WAITING 时才长出那个
/// 「我倒完了，回去吧」大按钮 —— 语义唯一，不跟左下角的「停止导航」混
/// （那个只在导航中出现，只表示取消导航）。
class TaskCard extends StatelessWidget {
  const TaskCard({super.key});

  @override
  Widget build(BuildContext context) {
    final task = context.read<TaskChannel>();
    return ValueListenableBuilder<TaskStatus?>(
      valueListenable: task.status,
      builder: (context, st, _) {
        // 连不上任务层（没起 my_bot_task）也整张隐藏，别在地图上挂个报错框
        if (st == null || !st.shouldShowCard) return const SizedBox.shrink();
        return Positioned(
          left: 0,
          right: 0,
          bottom: 16,
          child: SafeArea(
            top: false,
            child: Center(
              // ⚠️ 必须限宽。不限的话在桌面宽屏上会被拉成一条横跨整屏的巨条，
              // 那个绿色按钮会变成 2000px 宽。
              child: ConstrainedBox(
                constraints: const BoxConstraints(maxWidth: 380),
                child: Padding(
                  padding: const EdgeInsets.symmetric(horizontal: 12),
                  child: _Card(status: st, task: task),
                ),
              ),
            ),
          ),
        );
      },
    );
  }
}

class _Card extends StatelessWidget {
  const _Card({required this.status, required this.task});

  final TaskStatus status;
  final TaskChannel task;

  @override
  Widget build(BuildContext context) {
    return Material(
      elevation: 5,
      borderRadius: BorderRadius.circular(14),
      color: Colors.white,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(14, 11, 14, 11),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            _headline(),
            if (_subtitle() != null) ...[
              const SizedBox(height: 5),
              Text(_subtitle()!,
                  style: const TextStyle(fontSize: 11, color: Color(0xFF787E8A))),
            ],
            if (status.isWaiting) ...[
              const SizedBox(height: 10),
              _doneButton(context),
            ],
          ],
        ),
      ),
    );
  }

  Widget _headline() {
    const sub = TextStyle(fontSize: 12, color: Color(0xFF787E8A));
    const strong = TextStyle(
        fontSize: 15, fontWeight: FontWeight.w600, color: Color(0xFF1C1E24));

    if (status.isWaiting) {
      return Row(children: [
        const Icon(Icons.check_circle, size: 16, color: Color(0xFF22A860)),
        const SizedBox(width: 6),
        const Text('已到达', style: sub),
        const SizedBox(width: 5),
        Flexible(
            child: Text(status.currentName ?? '',
                style: strong, overflow: TextOverflow.ellipsis)),
        const Spacer(),
        Text('还剩 ${_mmss(status.dwellRemaining)}', style: sub),
      ]);
    }

    if (status.isStopped) {
      return const Row(children: [
        Icon(Icons.error_outline, size: 16, color: Color(0xFFD9534F)),
        SizedBox(width: 6),
        Expanded(child: Text('已停下', style: strong)),
      ]);
    }

    // NAVIGATING / RETURNING
    final returning = status.isReturning;
    return Row(children: [
      const SizedBox(
          width: 14,
          height: 14,
          child: CircularProgressIndicator(strokeWidth: 2)),
      const SizedBox(width: 8),
      Text(returning ? '正在回待命点…' : '正在前往', style: sub),
      if (!returning) ...[
        const SizedBox(width: 5),
        Flexible(
            child: Text(status.currentName ?? '',
                style: strong, overflow: TextOverflow.ellipsis)),
      ],
      const Spacer(),
      Text(_eta(),
          style: TextStyle(
              fontSize: returning ? 12 : 13,
              fontWeight: returning ? FontWeight.normal : FontWeight.w600,
              color: returning ? const Color(0xFF787E8A) : const Color(0xFF3478F6))),
    ]);
  }

  /// eta_sec == 0 表示 Nav2 的第一帧导航反馈还没到 —— 显示"计算中"，
  /// 别显示"约 0 秒"（那看起来像马上就到，但其实刚出发）。
  String _eta() {
    if (status.etaSec <= 0) return '计算中…';
    return '约 ${status.etaSec.round()} 秒';
  }

  String? _subtitle() {
    final parts = <String>[];
    if ((status.isNavigating || status.isReturning) &&
        status.distanceRemaining > 0) {
      parts.add('还有 ${status.distanceRemaining.toStringAsFixed(1)} 米');
    }
    if (status.queue.isNotEmpty) {
      parts.add('排队中：${status.queue.join('、')}');
    }
    if (status.isStopped && status.lastError.isNotEmpty) {
      return status.lastError;
    }
    if (status.isReturning && parts.isEmpty) return '队列为空';
    return parts.isEmpty ? null : parts.join(' · ');
  }

  Widget _doneButton(BuildContext context) {
    return SizedBox(
      height: 40,
      child: FilledButton.icon(
        style: FilledButton.styleFrom(
          backgroundColor: const Color(0xFF22A860),
          padding: EdgeInsets.zero,
          shape:
              RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
        ),
        icon: const Icon(Icons.check_circle, size: 17),
        label: const Text('我倒完了，回去吧',
            style: TextStyle(fontSize: 14, fontWeight: FontWeight.w600)),
        onPressed: () => task.skipWait(),
      ),
    );
  }

  static String _mmss(double sec) {
    final s = sec.round();
    return '${s ~/ 60}:${(s % 60).toString().padLeft(2, '0')}';
  }
}

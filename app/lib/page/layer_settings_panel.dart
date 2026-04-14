import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:ros_flutter_gui_app/basic/layer_config.dart';
import 'package:ros_flutter_gui_app/language/l10n/gen/app_localizations.dart';
import 'package:ros_flutter_gui_app/provider/global_state.dart';

Future<Color?> pickPresetColor(BuildContext context, Color current) async {
  const presets = <Color>[
    Colors.red,
    Colors.deepOrange,
    Colors.orange,
    Colors.amber,
    Colors.yellow,
    Colors.lime,
    Colors.green,
    Colors.teal,
    Colors.cyan,
    Colors.lightBlue,
    Colors.blue,
    Colors.indigo,
    Colors.purple,
    Colors.pink,
    Colors.white,
    Colors.black54,
  ];
  return showDialog<Color>(
    context: context,
    builder: (ctx) => AlertDialog(
      title: Text(AppLocalizations.of(ctx)!.layers),
      content: SizedBox(
        width: 280,
        child: Wrap(
          spacing: 6,
          runSpacing: 6,
          children: presets.map((c) {
            return InkWell(
              onTap: () => Navigator.pop(ctx, c),
              child: Container(
                width: 32,
                height: 32,
                decoration: BoxDecoration(
                  color: c,
                  border: Border.all(color: Colors.grey),
                  borderRadius: BorderRadius.circular(4),
                ),
              ),
            );
          }).toList(),
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(ctx),
          child: Text(AppLocalizations.of(ctx)!.cancel),
        ),
      ],
    ),
  );
}

class LayerSettingsPanel extends StatefulWidget {
  const LayerSettingsPanel({super.key});

  @override
  State<LayerSettingsPanel> createState() => _LayerSettingsPanelState();
}

class _LayerSettingsPanelState extends State<LayerSettingsPanel> {
  late double _laserDotDraft;

  final Set<String> _openLayerIds = <String>{};

  static const _dividerColor = Color(0x33000000);

  @override
  void initState() {
    super.initState();
    _laserDotDraft = Provider.of<GlobalState>(context, listen: false)
        .layerLaserDotRadius();
  }

  void _toggleOpen(String id) {
    setState(() {
      if (_openLayerIds.contains(id)) {
        _openLayerIds.remove(id);
      } else {
        _openLayerIds.add(id);
      }
    });
  }

  Widget _groupRowBorder({required Widget child}) {
    return DecoratedBox(
      decoration: const BoxDecoration(
        border: Border(
          bottom: BorderSide(color: _dividerColor, width: 0.5),
        ),
      ),
      child: child,
    );
  }

  Widget _buildLayerHeader(
    BuildContext context, {
    required String id,
    required String title,
    required String layerKey,
  }) {
    return Consumer<GlobalState>(
      builder: (ctx, gs, __) {
        final open = _openLayerIds.contains(id);
        final scheme = Theme.of(ctx).colorScheme;
        return _groupRowBorder(
          child: Material(
            color: Colors.transparent,
            child: InkWell(
              onTap: () => _toggleOpen(id),
              child: Padding(
                padding:
                    const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                child: Row(
                  children: [
                    Icon(
                      open ? Icons.expand_less : Icons.chevron_right,
                      size: 20,
                      color: Colors.grey,
                    ),
                    const SizedBox(width: 4),
                    Expanded(
                      child: Text(
                        title,
                        style: const TextStyle(
                          fontSize: 16,
                          color: Colors.black,
                        ),
                      ),
                    ),
                    Theme(
                      data: Theme.of(ctx).copyWith(
                        switchTheme: SwitchThemeData(
                          thumbColor:
                              WidgetStateProperty.resolveWith((states) {
                            if (states.contains(WidgetState.selected)) {
                              return scheme.onPrimary;
                            }
                            return const Color(0xFF616161);
                          }),
                          trackColor:
                              WidgetStateProperty.resolveWith((states) {
                            if (states.contains(WidgetState.selected)) {
                              return scheme.primary;
                            }
                            return const Color(0xFFE0E0E0);
                          }),
                        ),
                      ),
                      child: Switch.adaptive(
                        value: gs.isLayerVisible(layerKey),
                        onChanged: (v) => gs.setLayerState(layerKey, v),
                      ),
                    ),
                  ],
                ),
              ),
            ),
          ),
        );
      },
    );
  }

  Widget _buildColorRow(
    BuildContext context,
    GlobalState gs,
    String label,
    String layerId,
    Color fallback,
  ) {
    final c = gs.layerColorFor(layerId, fallback);
    return _groupRowBorder(
      child: Material(
        color: Colors.transparent,
        child: InkWell(
          onTap: () async {
            final p = await pickPresetColor(context, c);
            if (p != null) {
              gs.patchLayer(layerId, colorArgb: p.toARGB32().toString());
              await gs.saveLayerSettings();
              setState(() {});
            }
          },
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
            child: Row(
              children: [
                Expanded(
                  child: Text(
                    label,
                    style: const TextStyle(fontSize: 16, color: Colors.black),
                  ),
                ),
                Container(
                  width: 44,
                  height: 28,
                  decoration: BoxDecoration(
                    color: c,
                    borderRadius: BorderRadius.circular(6),
                    border: Border.all(color: Colors.grey.withValues(alpha: 0.35)),
                  ),
                ),
                const SizedBox(width: 4),
                const Icon(Icons.chevron_right, color: Colors.grey, size: 20),
              ],
            ),
          ),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;
    final gs = Provider.of<GlobalState>(context);

    return Column(
      mainAxisSize: MainAxisSize.min,
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _buildLayerHeader(context,
            id: 'grid', title: l10n.layer_grid, layerKey: 'grid'),
        _buildLayerHeader(context,
            id: 'lcost',
            title: l10n.layer_local_costmap,
            layerKey: 'localCostmap'),
        if (_openLayerIds.contains('lcost')) ...[
          Consumer<GlobalState>(
            builder: (ctx, gs, __) {
              final cfg = gs.layerConfig['localCostmap'];
              final style = cfg is LayerLocalCostmapConfig
                  ? cfg.mapStyle
                  : LocalCostmapMapStyle.costmap;
              return _groupRowBorder(
                child: Padding(
                  padding: const EdgeInsets.fromLTRB(16, 8, 16, 12),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Text(
                        l10n.local_costmap_map_style,
                        style: Theme.of(ctx).textTheme.bodySmall,
                      ),
                      const SizedBox(height: 8),
                      SegmentedButton<LocalCostmapMapStyle>(
                        showSelectedIcon: false,
                        segments: [
                          ButtonSegment(
                            value: LocalCostmapMapStyle.raw,
                            label: Text(l10n.local_costmap_style_raw),
                          ),
                          ButtonSegment(
                            value: LocalCostmapMapStyle.costmap,
                            label: Text(l10n.local_costmap_style_costmap),
                          ),
                          ButtonSegment(
                            value: LocalCostmapMapStyle.obs,
                            label: Text(l10n.local_costmap_style_obs),
                          ),
                        ],
                        selected: {style},
                        onSelectionChanged: (next) {
                          if (next.isEmpty) return;
                          gs.patchLayer(
                            'localCostmap',
                            mapStyle: next.first,
                          );
                          gs.saveLayerSettings();
                        },
                      ),
                    ],
                  ),
                ),
              );
            },
          ),
        ],
        _buildLayerHeader(context,
            id: 'laser', title: l10n.layer_laser, layerKey: 'laser'),
        if (_openLayerIds.contains('laser')) ...[
          _buildColorRow(context, gs, l10n.layer_color, 'laser', Colors.red),
          _groupRowBorder(
            child: Padding(
              padding: const EdgeInsets.fromLTRB(16, 4, 16, 8),
              child: Row(
                children: [
                  SizedBox(
                    width: 72,
                    child: Text(
                      l10n.layer_dot_size,
                      style: Theme.of(context).textTheme.bodySmall,
                    ),
                  ),
                  Expanded(
                    child: Slider.adaptive(
                      value: _laserDotDraft,
                      min: 0.5,
                      max: 8,
                      divisions: 15,
                      label: _laserDotDraft.toStringAsFixed(1),
                      onChanged: (v) {
                        setState(() => _laserDotDraft = v);
                      },
                      onChangeEnd: (_) {
                        gs.patchLayer(
                            'laser', dotRadius: _laserDotDraft.toString());
                        gs.saveLayerSettings();
                      },
                    ),
                  ),
                ],
              ),
            ),
          ),
        ],
        _buildLayerHeader(context,
            id: 'pc',
            title: l10n.layer_pointcloud,
            layerKey: 'pointCloud'),
        if (_openLayerIds.contains('pc')) ...[],
        _buildLayerHeader(context,
            id: 'gpath',
            title: l10n.layer_global_path,
            layerKey: 'globalPath'),
        if (_openLayerIds.contains('gpath')) ...[
          _buildColorRow(context, gs, l10n.layer_color, 'globalPath',
              const Color(0xFF2196F3)),
        ],
        _buildLayerHeader(context,
            id: 'lpath',
            title: l10n.layer_local_path,
            layerKey: 'localPath'),
        if (_openLayerIds.contains('lpath')) ...[
          _buildColorRow(
              context, gs, l10n.layer_color, 'localPath', Colors.green),
        ],
        _buildLayerHeader(context,
            id: 'tpath',
            title: l10n.layer_trace,
            layerKey: 'tracePath'),
        if (_openLayerIds.contains('tpath')) ...[
          _buildColorRow(
              context, gs, l10n.layer_color, 'tracePath', Colors.yellow),
        ],
        _buildLayerHeader(context,
            id: 'topo',
            title: l10n.layer_topology,
            layerKey: 'topology'),
        _buildLayerHeader(context,
            id: 'foot',
            title: l10n.layer_robot_footprint,
            layerKey: 'robotFootprint'),
        if (_openLayerIds.contains('foot')) ...[],
      ],
    );
  }
}
